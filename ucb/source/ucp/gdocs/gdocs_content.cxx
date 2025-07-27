/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertyAccess.hpp>
#include <com/sun/star/io/Pipe.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/lang/IllegalAccessException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/task/InteractionClassification.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include <com/sun/star/ucb/InsertCommandArgument.hpp>
#include <com/sun/star/ucb/InteractiveBadTransferURLException.hpp>
#include <com/sun/star/ucb/InteractiveAugmentedIOException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkResolveNameException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkConnectException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkReadException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkWriteException.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/ucb/UnsupportedDataSinkException.hpp>
#include <com/sun/star/ucb/UnsupportedNameClashException.hpp>
#include <com/sun/star/ucb/UnsupportedOpenModeException.hpp>
#include <com/sun/star/ucb/XCommandInfo.hpp>
#include <com/sun/star/ucb/XDynamicResultSet.hpp>

#include <comphelper/processfactory.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/propertyvalueset.hxx>
#include <ucbhelper/cancelcommandexecution.hxx>
#include <ucbhelper/macros.hxx>
#include <o3tl/runtimetooustring.hxx>
#include <sal/log.hxx>

#include "gdocs_content.hxx"
#include "gdocs_provider.hxx"
#include "gdocs_datasupplier.hxx"

#include <memory>
#include <string>
#include <sstream>

using namespace com::sun::star;

namespace gdocs
{

Content::Content( const uno::Reference< uno::XComponentContext >& rxContext,
    ContentProvider* pProvider,
    const uno::Reference< ucb::XContentIdentifier >& Identifier)
    : ContentImplHelper( rxContext, pProvider, Identifier )
    , m_pProvider( pProvider )
    , m_bTransient( false )
    , m_bIsFolder( false )
{
    SAL_INFO("ucb.ucp.gdocs", "Content::Content() " << m_xIdentifier->getContentIdentifier());
    
    m_sURL = m_xIdentifier->getContentIdentifier();
    
    // Parse the URL: gdocs://user@gmail.com/path/to/file
    if (m_sURL.startsWith("gdocs://"))
    {
        OUString sPath = m_sURL.copy(8); // Remove "gdocs://"
        sal_Int32 nPos = sPath.indexOf('/');
        if (nPos > 0)
        {
            m_sUserEmail = sPath.copy(0, nPos);
            m_sPath = sPath.copy(nPos);
        }
        else
        {
            m_sUserEmail = sPath;
            m_sPath = "/";
            m_bIsFolder = true;
        }
    }
}

Content::Content( const uno::Reference< uno::XComponentContext >& rxContext,
    ContentProvider* pProvider,
    const uno::Reference< ucb::XContentIdentifier >& Identifier,
    bool bIsFolder)
    : ContentImplHelper( rxContext, pProvider, Identifier )
    , m_pProvider( pProvider )
    , m_bTransient( true )
    , m_bIsFolder( bIsFolder )
{
    SAL_INFO("ucb.ucp.gdocs", "Content::Content() transient");
    m_sURL = m_xIdentifier->getContentIdentifier();
}

Content::~Content()
{
}

// XInterface methods.
void SAL_CALL Content::acquire() noexcept
{
    ContentImplHelper::acquire();
}

void SAL_CALL Content::release() noexcept
{
    ContentImplHelper::release();
}

css::uno::Any SAL_CALL Content::queryInterface( const css::uno::Type & rType )
{
    css::uno::Any aRet = cppu::queryInterface( rType,
        static_cast< ucb::XContentCreator * >( this ) );
    return aRet.hasValue() ? aRet : ContentImplHelper::queryInterface(rType);
}

// XTypeProvider methods.
css::uno::Sequence< sal_Int8 > SAL_CALL Content::getImplementationId()
{
    return css::uno::Sequence<sal_Int8>();
}

css::uno::Sequence< css::uno::Type > SAL_CALL Content::getTypes()
{
    static cppu::OTypeCollection s_aCollection(
        cppu::UnoType<lang::XTypeProvider>::get(),
        cppu::UnoType<lang::XServiceInfo>::get(),
        cppu::UnoType<lang::XComponent>::get(),
        cppu::UnoType<ucb::XContent>::get(),
        cppu::UnoType<ucb::XCommandProcessor>::get(),
        cppu::UnoType<beans::XPropertiesChangeNotifier>::get(),
        cppu::UnoType<ucb::XCommandInfoChangeNotifier>::get(),
        cppu::UnoType<beans::XPropertyContainer>::get(),
        cppu::UnoType<beans::XPropertySetInfoChangeNotifier>::get(),
        cppu::UnoType<container::XChild>::get(),
        cppu::UnoType<ucb::XContentCreator>::get()
    );

    return s_aCollection.getTypes();
}

// XServiceInfo methods.
OUString SAL_CALL Content::getImplementationName()
{
    return "com.sun.star.comp.GDocsContent";
}

uno::Sequence< OUString > SAL_CALL Content::getSupportedServiceNames()
{
    uno::Sequence<OUString> aSNS { "com.sun.star.ucb.GDocsContent" };
    return aSNS;
}

sal_Bool SAL_CALL Content::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService( this, ServiceName );
}

// XContent methods.
OUString SAL_CALL Content::getContentType()
{
    if (isFolder(uno::Reference< ucb::XCommandEnvironment >()))
        return GDOCS_FOLDER_TYPE;
    else
        return GDOCS_FILE_TYPE;
}

// XCommandProcessor methods.
uno::Any SAL_CALL Content::execute(
        const ucb::Command& aCommand,
        sal_Int32 /*CommandId*/,
        const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    SAL_INFO("ucb.ucp.gdocs", "Content::execute " << aCommand.Name);
    uno::Any aRet;

    if ( aCommand.Name == "getPropertyValues" )
    {
        uno::Sequence< beans::Property > Properties;
        if ( !( aCommand.Argument >>= Properties ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }
        aRet <<= getPropertyValues( Properties, xEnv );
    }
    else if ( aCommand.Name == "setPropertyValues" )
    {
        uno::Sequence< beans::PropertyValue > aProperties;
        if ( !( aCommand.Argument >>= aProperties ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }

        if ( !aProperties.hasElements() )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "No properties!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }

        aRet <<= setPropertyValues( aProperties, xEnv );
    }
    else if ( aCommand.Name == "getPropertySetInfo" )
    {
        aRet <<= getPropertySetInfo( xEnv );
    }
    else if ( aCommand.Name == "getCommandInfo" )
    {
        aRet <<= getCommandInfo( xEnv );
    }
    else if ( aCommand.Name == "open" )
    {
        ucb::OpenCommandArgument2 aOpenCommand;
        if ( !( aCommand.Argument >>= aOpenCommand ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }
        aRet = open( aOpenCommand, xEnv );
    }
    else if ( aCommand.Name == "insert" )
    {
        ucb::InsertCommandArgument aArg;
        if ( !( aCommand.Argument >>= aArg ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }
        insert( aArg.Data, aArg.ReplaceExisting, xEnv );
    }
    else if ( aCommand.Name == "delete" )
    {
        bool bDeletePhysical = false;
        aCommand.Argument >>= bDeletePhysical;
        destroy( bDeletePhysical, xEnv );
    }
    else if ( aCommand.Name == "transfer" )
    {
        ucb::TransferInfo transferArgs;
        if ( !( aCommand.Argument >>= transferArgs ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }
        transfer( transferArgs, xEnv );
    }
    else if ( aCommand.Name == "createNewContent" )
    {
        ucb::ContentInfo aArg;
        if ( !( aCommand.Argument >>= aArg ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                    "Wrong argument type!",
                    static_cast< cppu::OWeakObject * >( this ),
                    -1 ) ),
                xEnv );
        }
        aRet <<= createNewContent( aArg );
    }
    else
    {
        SAL_INFO("ucb.ucp.gdocs", "Unknown command to execute");

        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::UnsupportedCommandException(
                aCommand.Name,
                static_cast< cppu::OWeakObject * >( this ) ) ),
            xEnv );
    }

    return aRet;
}

void SAL_CALL Content::abort( sal_Int32 /*CommandId*/ )
{
    SAL_INFO("ucb.ucp.gdocs", "TODO - Content::abort()");
}

// XContentCreator methods.
uno::Sequence< ucb::ContentInfo > SAL_CALL Content::queryCreatableContentsInfo()
{
    if ( isFolder( uno::Reference< ucb::XCommandEnvironment >() ) )
    {
        // Folders can contain folders and files
        uno::Sequence<ucb::ContentInfo> seq(2);

        // Folder
        seq[0].Type = GDOCS_FOLDER_TYPE;
        seq[0].Attributes = ucb::ContentInfoAttribute::KIND_FOLDER;

        uno::Sequence< beans::Property > props( 1 );
        props[0] = beans::Property(
            "Title",
            -1,
            cppu::UnoType<OUString>::get(),
            beans::PropertyAttribute::BOUND );
        seq[0].Properties = props;

        // File
        seq[1].Type = GDOCS_FILE_TYPE;
        seq[1].Attributes = ucb::ContentInfoAttribute::KIND_DOCUMENT;
        seq[1].Properties = props;

        return seq;
    }
    else
    {
        return uno::Sequence< ucb::ContentInfo >();
    }
}

uno::Reference< ucb::XContent > SAL_CALL Content::createNewContent(
    const ucb::ContentInfo& Info )
{
    bool bIsFolder = Info.Type == GDOCS_FOLDER_TYPE;

    SAL_INFO("ucb.ucp.gdocs", "Content::createNewContent " << bIsFolder);

    OUString aURL = m_xIdentifier->getContentIdentifier();
    if ( ( aURL.lastIndexOf( '/' ) + 1 ) != aURL.getLength() )
        aURL += "/";

    aURL += "New_Content";

    uno::Reference< ucb::XContentIdentifier > xId(
        new ::ucbhelper::ContentIdentifier( aURL ) );

    try
    {
        return new Content( m_xContext, m_pProvider, xId, bIsFolder );
    }
    catch ( ucb::ContentCreationException & )
    {
        return uno::Reference< ucb::XContent >();
    }
}

// Non-interface methods
OUString Content::getParentURL()
{
    OUString sURL = m_xIdentifier->getContentIdentifier();

    // Find last '/'
    sal_Int32 nPos = sURL.lastIndexOf( '/' );
    if ( nPos > 0 )
    {
        OUString sParent = sURL.copy( 0, nPos );
        
        // If parent is just "gdocs://email", add trailing '/'
        if (sParent.indexOf('/', 8) == -1)
            sParent += "/";
            
        return sParent;
    }

    return OUString();
}

uno::Reference< sdbc::XRow > Content::getPropertyValues(
    const uno::Sequence< beans::Property >& rProperties,
    const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    rtl::Reference< ::ucbhelper::PropertyValueSet > xRow
        = new ::ucbhelper::PropertyValueSet( m_xContext );

    for ( const beans::Property& rProp : rProperties )
    {
        try
        {
            if ( rProp.Name == "IsDocument" )
            {
                xRow->appendBoolean( rProp, !isFolder(xEnv) );
            }
            else if ( rProp.Name == "IsFolder" )
            {
                xRow->appendBoolean( rProp, isFolder(xEnv) );
            }
            else if ( rProp.Name == "Title" )
            {
                OUString sTitle;
                if (!m_sTitle.empty())
                {
                    sTitle = OUString::fromUtf8(m_sTitle);
                }
                else if (m_pFile)
                {
                    sTitle = OUString::fromUtf8(m_pFile->name);
                }
                else
                {
                    // Extract title from URL
                    OUString sURL = m_xIdentifier->getContentIdentifier();
                    sal_Int32 nPos = sURL.lastIndexOf('/');
                    if (nPos >= 0)
                        sTitle = sURL.copy(nPos + 1);
                    else
                        sTitle = "Root";
                }
                xRow->appendString( rProp, sTitle );
            }
            else if ( rProp.Name == "IsReadOnly" )
            {
                xRow->appendBoolean( rProp, false );
            }
            else if ( rProp.Name == "DateCreated" )
            {
                xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "DateModified" )
            {
                xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "Size" )
            {
                if (m_pFile && !isFolder(xEnv))
                    xRow->appendLong( rProp, m_pFile->size );
                else
                    xRow->appendLong( rProp, 0 );
            }
            else if ( rProp.Name == "ContentType" )
            {
                xRow->appendString( rProp, getContentType() );
            }
            else
            {
                SAL_INFO("ucb.ucp.gdocs", "Unknown property: " << rProp.Name);
                xRow->appendVoid( rProp );
            }
        }
        catch (const uno::Exception&)
        {
            xRow->appendVoid( rProp );
        }
    }

    return xRow;
}

bool Content::isFolder(const uno::Reference< ucb::XCommandEnvironment >& /*xEnv*/)
{
    return m_bIsFolder;
}

std::shared_ptr<GoogleSession> Content::getSession(const uno::Reference< ucb::XCommandEnvironment >& /*xEnv*/)
{
    SAL_WARN("ucb.ucp.gdocs", "Content::getSession() called for user: " << m_sUserEmail);
    
    if (!m_pSession && m_pProvider)
    {
        SAL_WARN("ucb.ucp.gdocs", "No cached session, getting from provider");
        m_pSession = m_pProvider->getSession(m_sUserEmail);
        
        if (m_pSession)
        {
            SAL_WARN("ucb.ucp.gdocs", "Got session from provider - valid: " << m_pSession->isValid());
        }
        else
        {
            SAL_WARN("ucb.ucp.gdocs", "Provider returned null session");
        }
    }
    else if (m_pSession)
    {
        SAL_WARN("ucb.ucp.gdocs", "Using cached session - valid: " << m_pSession->isValid());
    }
    else
    {
        SAL_WARN("ucb.ucp.gdocs", "No provider available");
    }
    
    return m_pSession;
}

std::shared_ptr<GoogleFile> Content::getGoogleFile(const uno::Reference< ucb::XCommandEnvironment >& xEnv)
{
    if (!m_pFile && !m_sFileId.isEmpty())
    {
        auto pSession = getSession(xEnv);
        if (pSession && pSession->isValid())
        {
            OString sFileId = OUStringToOString(m_sFileId, RTL_TEXTENCODING_UTF8);
            m_pFile = getFile(*pSession, sFileId.getStr());
        }
    }
    return m_pFile;
}

uno::Any Content::open( const ucb::OpenCommandArgument2& rOpenCommand,
    const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    if ( rOpenCommand.Mode == ucb::OpenMode::ALL ||
         rOpenCommand.Mode == ucb::OpenMode::FOLDERS ||
         rOpenCommand.Mode == ucb::OpenMode::DOCUMENTS )
    {
        // For folder, return children
        if ( isFolder( xEnv ) )
        {
            uno::Reference< ucb::XDynamicResultSet > xSet
                = new DynamicResultSet( m_xContext, this, rOpenCommand );
            return uno::Any( xSet );
        }
        else
        {
            // For file, return content stream
            if ( rOpenCommand.Sink.is() )
            {
                uno::Reference< io::XOutputStream > xOut( rOpenCommand.Sink, uno::UNO_QUERY );
                uno::Reference< io::XActiveDataSink > xDataSink( rOpenCommand.Sink, uno::UNO_QUERY );
                
                if ( xOut.is() || xDataSink.is() )
                {
                    // Download the file
                    auto pSession = getSession(xEnv);
                    auto pFile = getGoogleFile(xEnv);
                    
                    if (pSession && pSession->isValid() && pFile)
                    {
                        // Create temporary file for download
                        OUString sTempFile;
                        ::osl::FileBase::createTempFile(nullptr, nullptr, &sTempFile);
                        
                        OString sTempPath;
                        ::osl::FileBase::getSystemPathFromFileURL(sTempFile, sTempFile);
                        sTempPath = OUStringToOString(sTempFile, RTL_TEXTENCODING_UTF8);
                        
                        if (downloadFile(*pSession, pFile->id, sTempPath.getStr()))
                        {
                            // TODO: Read file and write to output stream
                            SAL_INFO("ucb.ucp.gdocs", "File downloaded successfully");
                        }
                    }
                }
            }
        }
    }
    
    return uno::Any();
}

void Content::insert( const uno::Reference< io::XInputStream >& xInputStream,
    bool bReplaceExisting,
    const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    if ( !xInputStream.is() )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::MissingInputStreamException(
                OUString(),
                static_cast< cppu::OWeakObject * >( this ) ) ),
            xEnv );
    }

    // TODO: Implement file upload
    SAL_INFO("ucb.ucp.gdocs", "Content::insert - TODO");
}

void Content::transfer( const ucb::TransferInfo& rTransferInfo,
    const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    SAL_INFO("ucb.ucp.gdocs", "Content::transfer - TODO");
}

void Content::destroy( bool bDeletePhysical,
    const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    SAL_INFO("ucb.ucp.gdocs", "Content::destroy - TODO");
    
    if (bDeletePhysical)
    {
        auto pSession = getSession(xEnv);
        auto pFile = getGoogleFile(xEnv);
        
        if (pSession && pSession->isValid() && pFile)
        {
            if (deleteFile(*pSession, pFile->id))
            {
                SAL_INFO("ucb.ucp.gdocs", "File deleted successfully");
            }
        }
    }
}

uno::Sequence< uno::Any > Content::setPropertyValues(
    const uno::Sequence< beans::PropertyValue >& rValues,
    const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    uno::Sequence< uno::Any > aRet( rValues.getLength() );
    auto aRetRange = asNonConstRange(aRet);
    
    beans::PropertyChangeEvent aEvent;
    aEvent.Source = static_cast< cppu::OWeakObject * >( this );
    aEvent.Further = false;
    aEvent.PropertyHandle = -1;
    
    for ( sal_Int32 n = 0; n < rValues.getLength(); ++n )
    {
        const beans::PropertyValue& rValue = rValues[ n ];
        
        if ( rValue.Name == "Title" )
        {
            OUString sNewTitle;
            if ( rValue.Value >>= sNewTitle )
            {
                if ( !sNewTitle.isEmpty() )
                {
                    OString sTitle = OUStringToOString(sNewTitle, RTL_TEXTENCODING_UTF8);
                    m_sTitle = sTitle.getStr();
                    aEvent.PropertyName = "Title";
                    aEvent.OldValue <<= OUString::fromUtf8(m_sTitle);
                    aEvent.NewValue <<= sNewTitle;
                    aRetRange[ n ] <<= getBadArgExcept();
                }
                else
                {
                    aRetRange[ n ] <<= lang::IllegalArgumentException(
                        "Empty Title not allowed!",
                        static_cast< cppu::OWeakObject * >( this ),
                        -1 );
                }
            }
            else
            {
                aRetRange[ n ] <<= beans::IllegalTypeException(
                    "Title property value has wrong type!",
                    static_cast< cppu::OWeakObject * >( this ) );
            }
        }
        else
        {
            aRetRange[ n ] <<= beans::UnknownPropertyException(
                "Property is unknown!",
                static_cast< cppu::OWeakObject * >( this ) );
        }
    }
    
    return aRet;
}

uno::Any Content::getBadArgExcept()
{
    return uno::Any( lang::IllegalArgumentException(
        "Wrong argument type!",
        static_cast< cppu::OWeakObject * >( this ),
        -1 ) );
}

uno::Sequence< beans::Property > Content::getProperties(
    const uno::Reference< ucb::XCommandEnvironment > & /*xEnv*/ )
{
    static const beans::Property aGenericProperties[] =
    {
        beans::Property( "ContentType",
            -1, cppu::UnoType<OUString>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "IsDocument",
            -1, cppu::UnoType<bool>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "IsFolder",
            -1, cppu::UnoType<bool>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "Title",
            -1, cppu::UnoType<OUString>::get(),
            beans::PropertyAttribute::BOUND ),
        beans::Property( "IsReadOnly",
            -1, cppu::UnoType<bool>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "DateCreated",
            -1, cppu::UnoType<util::DateTime>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "DateModified",
            -1, cppu::UnoType<util::DateTime>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        beans::Property( "Size",
            -1, cppu::UnoType<sal_Int64>::get(),
            beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY )
    };

    const int nProps = SAL_N_ELEMENTS(aGenericProperties);
    return uno::Sequence< beans::Property > ( aGenericProperties, nProps );
}

Content::ContentRefList Content::getChildren( )
{
    ContentRefList aChildren;
    
    // TODO: Implement children listing using Google Drive API
    
    return aChildren;
}

void Content::copyData(
    const uno::Reference< io::XInputStream >& xIn,
    const uno::Reference< io::XOutputStream >& xOut )
{
    const sal_Int32 BUF_SIZE = 32768;
    uno::Sequence< sal_Int8 > aBuffer( BUF_SIZE );

    while (true)
    {
        sal_Int32 nRead = xIn->readBytes( aBuffer, BUF_SIZE );
        if ( nRead < BUF_SIZE )
        {
            aBuffer.realloc( nRead );
            xOut->writeBytes( aBuffer );
            break;
        }
        else
            xOut->writeBytes( aBuffer );
    }

    xOut->closeOutput();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */