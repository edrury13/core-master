/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cppuhelper/supportsservice.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <com/sun/star/ucb/XContentIdentifier.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <comphelper/processfactory.hxx>
#include <osl/diagnose.h>

#include "gdocs_provider.hxx"
#include "gdocs_content.hxx"
#include "gdocs_auth.hxx"

using namespace com::sun::star;

namespace gdocs
{

ContentProvider::ContentProvider(
    const uno::Reference< uno::XComponentContext >& rxContext )
: ::ucbhelper::ContentProviderImplHelper( rxContext )
{
}

ContentProvider::~ContentProvider()
{
}

// XInterface methods.
css::uno::Any SAL_CALL ContentProvider::queryInterface( const css::uno::Type & rType )
{
    css::uno::Any aRet = cppu::queryInterface( rType,
        static_cast< lang::XTypeProvider* >(this),
        static_cast< lang::XServiceInfo* >(this),
        static_cast< ucb::XContentProvider* >(this)
    );
    return aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType );
}

void SAL_CALL ContentProvider::acquire() noexcept
{
    OWeakObject::acquire();
}

void SAL_CALL ContentProvider::release() noexcept
{
    OWeakObject::release();
}

// XTypeProvider methods.
css::uno::Sequence< sal_Int8 > SAL_CALL ContentProvider::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

css::uno::Sequence< css::uno::Type > SAL_CALL ContentProvider::getTypes()
{
    static cppu::OTypeCollection s_aCollection(
        cppu::UnoType<lang::XTypeProvider>::get(),
        cppu::UnoType<lang::XServiceInfo>::get(),
        cppu::UnoType<ucb::XContentProvider>::get()
    );

    return s_aCollection.getTypes();
}

// XServiceInfo methods.
OUString SAL_CALL ContentProvider::getImplementationName()
{
    return "com.sun.star.comp.GDocsContentProvider";
}

sal_Bool SAL_CALL ContentProvider::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

css::uno::Sequence< OUString > SAL_CALL ContentProvider::getSupportedServiceNames()
{
    return { "com.sun.star.ucb.GDocsContentProvider" };
}

// XContentProvider methods.
uno::Reference< ucb::XContent > SAL_CALL ContentProvider::queryContent(
    const uno::Reference< ucb::XContentIdentifier >& Identifier )
{
    // Check URL scheme. Should be like:
    // gdocs://user@gmail.com/
    // gdocs://user@gmail.com/folder/Document.docx

    OUString aScheme;
    OUString aURL = Identifier->getContentIdentifier();
    
    if ( aURL.isEmpty() )
        throw ucb::IllegalIdentifierException();

    sal_Int32 nPos = aURL.indexOf( ':' );
    if ( nPos != -1 )
    {
        aScheme = aURL.copy( 0, nPos ).toAsciiLowerCase();
        if ( aScheme != "gdocs" )
            throw ucb::IllegalIdentifierException();
    }
    else
    {
        throw ucb::IllegalIdentifierException();
    }

    uno::Reference< ucb::XContent > xContent;
    try
    {
        xContent = new Content( m_xContext, this, Identifier );
    }
    catch ( ucb::ContentCreationException const & )
    {
        throw ucb::IllegalIdentifierException();
    }

    return xContent;
}

std::shared_ptr<GoogleSession> ContentProvider::getSession( const OUString& sUserEmail )
{
    SAL_WARN("ucb.ucp.gdocs", "ContentProvider::getSession() called for: " << sUserEmail);
    
    OString sKey = OUStringToOString( sUserEmail, RTL_TEXTENCODING_UTF8 );
    auto it = m_aSessionCache.find( sKey.getStr() );
    
    if ( it != m_aSessionCache.end() )
    {
        SAL_WARN("ucb.ucp.gdocs", "Found cached session - valid: " << (it->second ? it->second->isValid() : false));
        return it->second;
    }
    
    SAL_WARN("ucb.ucp.gdocs", "No cached session, creating new one");
    // Create new session
    std::shared_ptr<GoogleSession> pSession = createGoogleSession( sUserEmail );
    if ( pSession )
    {
        SAL_WARN("ucb.ucp.gdocs", "Created new session - valid: " << pSession->isValid());
        m_aSessionCache[ sKey.getStr() ] = pSession;
    }
    else
    {
        SAL_WARN("ucb.ucp.gdocs", "Failed to create session");
    }
    
    return pSession;
}

void ContentProvider::registerSession( const OUString& sUserEmail, std::shared_ptr<GoogleSession> pSession )
{
    OString sKey = OUStringToOString( sUserEmail, RTL_TEXTENCODING_UTF8 );
    m_aSessionCache[ sKey.getStr() ] = pSession;
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
ucb_gdocs_ContentProvider_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new gdocs::ContentProvider(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */