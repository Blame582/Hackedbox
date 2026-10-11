
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

struct DesktopApp {
    std::string filename;
    std::string name;
    std::string exec;
    std::string icon;
    std::string category;
};

static std::string trim(const std::string &s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

static std::string lower(std::string s) {
    for (char &c : s)
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool truth(const std::string &s) {
    return lower(trim(s)) == "true";
}

static std::string menuText(std::string s) {
    for (char &c : s) {
        if (c == '\n' || c == '\r' || c == '(' ||
            c == ')' || c == '{' || c == '}' ||
            c == '<' || c == '>')
            c = ' ';
    }
    return trim(s);
}

static std::string shellQuote(const std::string &s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'')
            out += "'\\''";
        else
            out += c;
    }
    out += "'";
    return out;
}

static std::vector<std::string> splitExec(
    const std::string &exec,
    const std::string &name,
    const std::string &desktopPath)
{
    std::vector<std::string> args;
    std::string token;
    char quote = 0;
    bool escaped = false;

    for (char c : exec) {
        if (escaped) {
            token += c;
            escaped = false;
        } else if (c == '\\' && quote != '\'') {
            escaped = true;
        } else if (quote) {
            if (c == quote)
                quote = 0;
            else
                token += c;
        } else if (c == '\'' || c == '"') {
            quote = c;
        } else if (std::isspace(
                       static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                args.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }

    if (escaped)
        token += '\\';

    if (!token.empty())
        args.push_back(token);

    std::vector<std::string> result;

    for (std::string arg : args) {
        bool discard = false;
        std::string expanded;

        for (size_t i = 0; i < arg.size(); ++i) {
            if (arg[i] != '%' || i + 1 >= arg.size()) {
                expanded += arg[i];
                continue;
            }

            char code = arg[++i];

            if (code == '%') {
                expanded += '%';
            } else if (code == 'c') {
                expanded += name;
            } else if (code == 'k') {
                expanded += desktopPath;
            } else if (std::string("fFuUdDnNivm").find(code)
                       != std::string::npos) {
                discard = true;
                break;
            } else {
                discard = true;
                break;
            }
        }

        if (!discard && !expanded.empty())
            result.push_back(expanded);
    }

    return result;
}

static std::string makeCommand(
    const std::string &exec,
    const std::string &name,
    const std::string &desktopPath)
{
    std::vector<std::string> args =
        splitExec(exec, name, desktopPath);

    std::string command;

    for (const std::string &arg : args) {
        if (!command.empty())
            command += ' ';
        command += shellQuote(arg);
    }

    return command;
}

static std::string firstCategory(const std::string &categories) {
    size_t start = 0;

    while (start < categories.size()) {
        size_t end = categories.find(';', start);
        if (end == std::string::npos)
            end = categories.size();

        std::string category =
            trim(categories.substr(start, end - start));

        if (!category.empty())
            return menuText(category);

        start = end + 1;
    }

    return "Other";
}

static std::set<std::string> readBlacklist(
    const std::string &path)
{
    std::set<std::string> blacklist;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        blacklist.insert(line);
    }

    return blacklist;
}

static bool readDesktop(
    const std::string &path,
    const std::set<std::string> &blacklist,
    DesktopApp &app)
{
    size_t slash = path.find_last_of('/');
    app.filename = slash == std::string::npos
        ? path : path.substr(slash + 1);

    if (blacklist.count(app.filename))
        return false;

    std::ifstream file(path);
    if (!file)
        return false;

    std::map<std::string, std::string> fields;
    std::string line;
    bool inEntry = false;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line.front() == '[' && line.back() == ']') {
            inEntry = line == "[Desktop Entry]";
            continue;
        }

        if (!inEntry)
            continue;

        size_t equals = line.find('=');
        if (equals == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, equals));
        std::string value = trim(line.substr(equals + 1));

        fields[key] = value;
    }

    if (fields["Type"] != "Application" ||
        truth(fields["Hidden"]) ||
        truth(fields["NoDisplay"]))
        return false;

    app.name = fields["Name"];
    app.exec = fields["Exec"];
    app.icon = fields["Icon"];
    app.category = firstCategory(fields["Categories"]);

    app.name = menuText(app.name);
    app.icon = menuText(app.icon);

    if (app.name.empty() || app.exec.empty())
        return false;

    app.exec = makeCommand(app.exec, app.name, path);

    return !app.exec.empty();
}

static void collectDirectory(
    const std::string &directory,
    const std::set<std::string> &blacklist,
    std::set<std::string> &seen,
    std::vector<DesktopApp> &apps)
{
    DIR *dir = opendir(directory.c_str());
    if (!dir)
        return;

    std::vector<std::string> files;
    struct dirent *entry;

    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        if (name.size() > 8 &&
            name.substr(name.size() - 8) == ".desktop")
            files.push_back(name);
    }

    closedir(dir);
    std::sort(files.begin(), files.end());

    for (const std::string &name : files) {
        if (seen.count(name) || blacklist.count(name))
            continue;

        std::string path = directory + "/" + name;
        struct stat st;

        if (stat(path.c_str(), &st) != 0 ||
            !S_ISREG(st.st_mode))
            continue;

        seen.insert(name);

        DesktopApp app;
        if (readDesktop(path, blacklist, app))
            apps.push_back(app);
    }
}

static bool writeMenu(
    const std::string &path,
    const std::vector<DesktopApp> &apps)
{
    std::ofstream out(path);
    if (!out)
        return false;

    std::map<std::string, std::vector<DesktopApp>> groups;

    for (const DesktopApp &app : apps)
        groups[app.category].push_back(app);

    out << "# Generated by Hackedbox menuGen.\n";
    out << "# Do not edit; regenerate with menuGen.\n";
    out << "[begin] (Applications)\n";

    for (auto &group : groups) {
        auto &entries = group.second;

        std::sort(entries.begin(), entries.end(),
            [](const DesktopApp &a, const DesktopApp &b) {
                return lower(a.name) < lower(b.name);
            });

        out << "[submenu] (" << menuText(group.first) << ")\n";

        for (const DesktopApp &app : entries) {
            out << "[exec] (" << menuText(app.name) << ") {"
                << app.exec << "}";

            if (!app.icon.empty())
                out << " <" << app.icon << ">";

            out << '\n';
        }

        out << "[end]\n";
    }

    out << "[end]\n";
    out.close();

    return !out.fail();
}

int main() {
    const char *home = std::getenv("HOME");

    if (!home || !*home) {
        std::cerr << "menuGen: HOME is not set\n";
        return 1;
    }

    std::string config = std::string(home) + "/.hackedbox";
    std::string blacklistPath = config + "/menu.blacklist";
    std::string outputPath = config + "/menu.generated.rc";

    struct stat st;
    if (stat(config.c_str(), &st) != 0 ||
        !S_ISDIR(st.st_mode)) {
        std::cerr << "menuGen: " << config
                  << " does not exist\n";
        return 1;
    }

    const std::set<std::string> blacklist =
        readBlacklist(blacklistPath);

    std::set<std::string> seen;
    std::vector<DesktopApp> apps;

    // User entries take precedence over system entries.
    collectDirectory(config.empty() ? "" :
        std::string(home) + "/.local/share/applications",
        blacklist, seen, apps);
    collectDirectory("/usr/local/share/applications",
        blacklist, seen, apps);
    collectDirectory("/usr/share/applications",
        blacklist, seen, apps);

    if (!writeMenu(outputPath, apps)) {
        std::cerr << "menuGen: cannot write "
                  << outputPath << '\n';
        return 1;
    }

    std::cout << "menuGen: wrote " << apps.size()
              << " applications to " << outputPath << '\n';

    return 0;
}
