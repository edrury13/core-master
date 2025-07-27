/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <rtl/ref.hxx>
#include <ucbhelper/resultset.hxx>
#include <ucbhelper/resultsethelper.hxx>

namespace gdocs
{

class Content;

struct ResultListEntry
{
    css::uno::Reference< css::ucb::XContent > xContent;
    css::uno::Reference< css::sdbc::XRow > xRow;

    explicit ResultListEntry( css::uno::Reference< css::ucb::XContent > xCnt ) : xContent( std::move(xCnt) )
    {
    }
};

class DataSupplier : public ucbhelper::ResultSetDataSupplier
{
private:
    rtl::Reference< Content > m_xContent;
    sal_Int32 m_nOpenMode;
    bool m_bCountFinal;
    void getData();
    std::vector< ResultListEntry > m_aResults;

public:
    DataSupplier( const rtl::Reference< Content >& rContent, sal_Int32 nOpenMode );

    virtual ~DataSupplier() override;

    virtual OUString queryContentIdentifierString( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) override;
    virtual css::uno::Reference< css::ucb::XContentIdentifier >
        queryContentIdentifier( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) override;
    virtual css::uno::Reference< css::ucb::XContent >
        queryContent( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) override;

    virtual bool getResult( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex ) override;

    virtual sal_uInt32 totalCount(std::unique_lock<std::mutex>& rResultSetGuard) override;
    virtual sal_uInt32 currentCount() override;
    virtual bool isCountFinal() override;

    virtual css::uno::Reference< css::sdbc::XRow >
        queryPropertyValues( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex  ) override;
    virtual void releasePropertyValues( sal_uInt32 nIndex ) override;

    virtual void close() override;

    virtual void validate() override;
};

class DynamicResultSet : public ::ucbhelper::ResultSetImplHelper
{
    rtl::Reference< Content > m_xContent;
    css::uno::Reference< css::ucb::XCommandEnvironment > m_xEnv;

private:
    virtual void initStatic() override;
    virtual void initDynamic() override;

public:
    DynamicResultSet(
        const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        const rtl::Reference< Content >& rxContent,
        const css::ucb::OpenCommandArgument2& rCommand );
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */