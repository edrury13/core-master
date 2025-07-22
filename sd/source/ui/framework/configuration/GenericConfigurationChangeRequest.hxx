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

#include <framework/ConfigurationChangeRequest.hxx>
#include <com/sun/star/container/XNamed.hpp>
#include <comphelper/compbase.hxx>

namespace sd::framework {
class ResourceId;

typedef cppu::ImplInheritanceHelper <
      sd::framework::ConfigurationChangeRequest,
      css::container::XNamed
    > GenericConfigurationChangeRequestInterfaceBase;

/** This implementation of the ConfigurationChangeRequest interface
    represents a single explicit request for a configuration change.  On its
    execution it may result in other, implicit, configuration changes.  For
    example this is the case when the deactivation of a unique resource is
    requested: the resources linked to it have to be deactivated as well.
*/
class GenericConfigurationChangeRequest final
    : public GenericConfigurationChangeRequestInterfaceBase
{
public:
    /** This enum specified whether the activation or deactivation of a
        resource is requested.
    */
    enum Mode { Activation, Deactivation };

    /** Create a new object that represents the request for activation or
        deactivation of the specified resource.
        @param rxsResourceId
            Id of the resource that is to be activated or deactivated.
        @param eMode
            The mode specifies whether to activate or to deactivate the
            resource.
        @throws css::css::lang::IllegalArgumentException
    */
    GenericConfigurationChangeRequest (
        const rtl::Reference<sd::framework::ResourceId>&
            rxResourceId,
        const Mode eMode);

    virtual ~GenericConfigurationChangeRequest() noexcept override;

    // XConfigurationChangeOperation

    /** The requested configuration change is executed on the given
        configuration.  Additionally to the explicitly requested change
        other changes have to be made as well.  See class description for an
        example.
        @param rxConfiguration
            The configuration to which the requested change is made.
    */
    virtual void execute (
        const rtl::Reference<sd::framework::Configuration>& rxConfiguration) override;

    // XNamed

    /** Return a human readable string representation.  This is used for
        debugging purposes.
    */
    virtual OUString SAL_CALL getName() override;

    /** This call is ignored because the XNamed interface is (mis)used to
        give access to a human readable name for debugging purposes.
    */
    virtual void SAL_CALL setName (const OUString& rName) override;

private:
    const rtl::Reference<sd::framework::ResourceId> mxResourceId;
    const Mode meMode;
};

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
