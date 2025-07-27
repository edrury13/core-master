/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/ucb/OpenMode.hpp>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/providerhelper.hxx>
#include <ucbhelper/macros.hxx>

#include "gdocs_datasupplier.hxx"
#include "gdocs_content.hxx"
#include "gdocs_provider.hxx"

using namespace com::sun::star;

namespace gdocs
{

DataSupplier::DataSupplier( const rtl::Reference< Content >& rContent, sal_Int32 nOpenMode )
    : m_xContent( rContent )
    , m_nOpenMode( nOpenMode )
    , m_bCountFinal( false )
{
}

DataSupplier::~DataSupplier()
{
}

OUString DataSupplier::queryContentIdentifierString( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    return queryContentIdentifier( rResultSetGuard, nIndex )->getContentIdentifier();
}

uno::Reference< ucb::XContentIdentifier > DataSupplier::queryContentIdentifier( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    auto xContent = queryContent( rResultSetGuard, nIndex );
    if ( xContent.is() )
        return xContent->getIdentifier();
    else
        return uno::Reference< ucb::XContentIdentifier >();
}

uno::Reference< ucb::XContent > DataSupplier::queryContent( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    if (!getResult(rResultSetGuard, nIndex))
        return uno::Reference< ucb::XContent >();

    return m_aResults[ nIndex ].xContent;
}

bool DataSupplier::getResult( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    if ( m_aResults.size() > nIndex ) // Result already present.
        return true;

    getData();

    if ( m_aResults.size() > nIndex ) // Result present after getData.
        return true;

    // No result for nIndex present.
    return false;
}

sal_uInt32 DataSupplier::totalCount(std::unique_lock<std::mutex>& rResultSetGuard)
{
    getData();
    return m_aResults.size();
}

sal_uInt32 DataSupplier::currentCount()
{
    return m_aResults.size();
}

bool DataSupplier::isCountFinal()
{
    return m_bCountFinal;
}

uno::Reference< sdbc::XRow > DataSupplier::queryPropertyValues( std::unique_lock<std::mutex>& rResultSetGuard, sal_uInt32 nIndex )
{
    if ( nIndex < m_aResults.size() )
    {
        uno::Reference< sdbc::XRow > xRow = m_aResults[ nIndex ].xRow;
        if ( xRow.is() )
        {
            // Already cached.
            return xRow;
        }
    }

    if ( getResult( rResultSetGuard, nIndex ) )
    {
        uno::Reference< ucb::XContent > xContent( m_aResults[ nIndex ].xContent );
        if ( xContent.is() )
        {
            try
            {
                uno::Reference< ucb::XCommandProcessor > xCmdProc(
                    xContent, uno::UNO_QUERY_THROW );
                sal_Int32 nCmdId( xCmdProc->createCommandIdentifier() );
                ucb::Command aCmd;
                aCmd.Name = "getPropertyValues";
                aCmd.Handle = -1;
                aCmd.Argument <<= getResultSet()->getProperties();
                uno::Any aResult( xCmdProc->execute(
                    aCmd, nCmdId, getResultSet()->getEnvironment() ) );
                uno::Reference< sdbc::XRow > xRow;
                if ( aResult >>= xRow )
                {
                    m_aResults[ nIndex ].xRow = xRow;
                    return xRow;
                }
            }
            catch ( uno::Exception const & )
            {
            }
        }
    }
    return uno::Reference< sdbc::XRow >();
}

void DataSupplier::releasePropertyValues( sal_uInt32 nIndex )
{
    if ( nIndex < m_aResults.size() )
        m_aResults[ nIndex ].xRow.clear();
}

void DataSupplier::close()
{
}

void DataSupplier::validate()
{
}

void DataSupplier::getData()
{
    SAL_WARN("ucb.ucp.gdocs", "DataSupplier::getData() called");
    
    if ( m_bCountFinal )
    {
        SAL_WARN("ucb.ucp.gdocs", "DataSupplier::getData() - already final, returning");
        return;
    }

    try
    {
        // Get the session and file information
        SAL_WARN("ucb.ucp.gdocs", "Getting session from content");
        auto pSession = m_xContent->getSession(uno::Reference< ucb::XCommandEnvironment >());
        auto pFile = m_xContent->getGoogleFile(uno::Reference< ucb::XCommandEnvironment >());
        
        if (!pSession)
        {
            SAL_WARN("ucb.ucp.gdocs", "No session available");
            m_bCountFinal = true;
            return;
        }
        
        if (!pSession->isValid())
        {
            SAL_WARN("ucb.ucp.gdocs", "Session is invalid");
            m_bCountFinal = true;
            return;
        }
        
        SAL_WARN("ucb.ucp.gdocs", "Session is valid, proceeding to list files");

        // List files in current folder
        std::string folderId;
        if (pFile)
        {
            folderId = pFile->id;
            SAL_WARN("ucb.ucp.gdocs", "Listing files in folder: " << folderId);
        }
        else
        {
            SAL_WARN("ucb.ucp.gdocs", "Listing files in root folder");
        }
            
        std::vector<GoogleFile> files = listFiles(*pSession, folderId);
        SAL_WARN("ucb.ucp.gdocs", "Retrieved " << files.size() << " files from Google Drive");
        
        for (const auto& file : files)
        {
            // Check if we should include this based on OpenMode
            bool bInclude = false;
            if (m_nOpenMode == ucb::OpenMode::ALL)
                bInclude = true;
            else if (m_nOpenMode == ucb::OpenMode::FOLDERS && file.isFolder)
                bInclude = true;
            else if (m_nOpenMode == ucb::OpenMode::DOCUMENTS && !file.isFolder)
                bInclude = true;
                
            if (bInclude)
            {
                // Create content URL
                OUString sURL = m_xContent->getIdentifier()->getContentIdentifier();
                if (!sURL.endsWith("/"))
                    sURL += "/";
                sURL += OUString::fromUtf8(file.name);
                
                // Create content identifier
                uno::Reference< ucb::XContentIdentifier > xId 
                    = new ::ucbhelper::ContentIdentifier(sURL);
                
                // Create content
                try
                {
                    SAL_WARN("ucb.ucp.gdocs", "Creating content for: " << sURL);
                    uno::Reference< ucb::XContent > xContent 
                        = new Content(m_xContent->getContext(),
                                    static_cast<ContentProvider*>(m_xContent->getProvider().get()),
                                    xId,
                                    file.isFolder);
                    
                    m_aResults.emplace_back(xContent);
                }
                catch (const ucb::ContentCreationException& e)
                {
                    SAL_WARN("ucb.ucp.gdocs", "Failed to create content: " << e.Message);
                }
            }
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Exception in getData: " << e.Message);
    }
    
    SAL_WARN("ucb.ucp.gdocs", "DataSupplier::getData() completed with " << m_aResults.size() << " results");
    m_bCountFinal = true;
}

// DynamicResultSet Implementation
DynamicResultSet::DynamicResultSet(
    const uno::Reference< uno::XComponentContext >& rxContext,
    const rtl::Reference< Content >& rxContent,
    const ucb::OpenCommandArgument2& rCommand )
    : ResultSetImplHelper( rxContext, rCommand )
    , m_xContent( rxContent )
    , m_xEnv( rCommand.Environment )
{
}

void DynamicResultSet::initStatic()
{
    m_xResultSet1
        = new ::ucbhelper::ResultSet(
            m_xContext,
            m_aCommand.Properties,
            new DataSupplier( m_xContent, m_aCommand.Mode ),
            m_xEnv );
}

void DynamicResultSet::initDynamic()
{
    m_xResultSet2 = m_xResultSet1;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */