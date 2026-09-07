// Runbox.cpp for Hackedbox - an XLibre Window Manager
// Copyright (c) 2026 Kevin Day <blame582@gmail.com>
// See AUTHORS for additional contributors and historical copyright holders.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <Runbox.hpp>
#include "Util.hpp"

#include <Button.hpp>
#include <Entry.hpp>
#include <WidgetContainer.hpp>

#include <string>


Runbox::Runbox()
    : HbTK::Dialog(
          0,
          0,
          520,
          150
      ),
      m_entry(nullptr),
      m_runButton(nullptr),
      m_cancelButton(nullptr)
{
    setTitle("Run");

    auto* container =
        new HbTK::WidgetContainer();

    container->setGeometry(
        0,
        0,
        520,
        150
    );

    m_entry =
        new HbTK::Entry(
            20,
            20,
            480,
            32
        );

    m_entry->setPlaceholder(
        "Enter command..."
    );

    m_runButton =
        new HbTK::Button(
            390,
            70,
            110,
            32,
            "Run"
        );

    m_cancelButton =
        new HbTK::Button(
            270,
            70,
            110,
            32,
            "Cancel"
        );

    container->addWidget(m_entry);
    container->addWidget(m_cancelButton);
    container->addWidget(m_runButton);

    setCentralWidget(container);
}

Runbox::~Runbox()
{
}

void Runbox::showRunbox()
{
    if (m_entry)
    {
        m_entry->clear();
        m_entry->setFocus(true);
    }

    show();
}

void Runbox::run()
{
    if (!m_entry)
        return;

    const std::string command =
        m_entry->text();

    if (command.empty())
        return;

    /*
     * displayString will be supplied by
     * Hackedbox when Runbox is integrated.
     */
}

void Runbox::cancel()
{
    hide();
}
