/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#pragma once

#include <framework/AbstractResource.hxx>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/rendering/XCanvas.hpp>
#include <sddllapi.h>

namespace sd::framework
{
/** A pane is an abstraction of a window and is one of the resources managed
    by the drawing framework.
    <p>Apart from the area that displays a view a pane may contain other
    parts like title, menu, close button.</p>
    <p>The URL prefix of panes is <code>private:resource/floater</code></p>
*/
class SD_DLLPUBLIC AbstractPane : public AbstractResource
{
public:
    virtual ~AbstractPane() override;

    /** Return the com::sun::star::awt::XWindow of the
        pane that is used to display a view.
    */
    virtual css::uno::Reference<css::awt::XWindow> getWindow() = 0;

    /** Return the com::sun::star::awt::XCanvas of the pane.  The
        com::sun::star::rendering::XCanvas object is expected to
        be associated with the com::sun::star::awt::XWindow object returned by
        getWindow().
        @return
            When the com::sun::star::rendering::XCanvas
            interface is not supported then an empty reference is returned.
    */
    virtual css::uno::Reference<css::rendering::XCanvas> getCanvas() = 0;
};

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
