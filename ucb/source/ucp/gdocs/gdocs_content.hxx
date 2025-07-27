/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>
#include <string_view>

#include "gdocs_auth.hxx"

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/ucb/CheckinArgument.hpp>
#include <com/sun/star/ucb/CommandFailedException.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/XContentCreator.hpp>
#include <ucbhelper/contenthelper.hxx>

namespace com::sun::star {
    namespace beans {
        struct Property;
        struct PropertyValue;
    }
    namespace sdbc {
        class XRow;
    }
}
namespace ucbhelper
{
    class Content;
}

namespace gdocs
{

inline constexpr OUString GDOCS_FILE_TYPE = u"application/vnd.libreoffice.gdocs-file"_ustr;
inline constexpr OUString GDOCS_FOLDER_TYPE = u"application/vnd.libreoffice.gdocs-folder"_ustr;

class ContentProvider;
class Content : public ::ucbhelper::ContentImplHelper,
                public css::ucb::XContentCreator
{
private:
    ContentProvider*              m_pProvider;
    std::shared_ptr<GoogleSession> m_pSession;
    std::shared_ptr<GoogleFile>    m_pFile;
    OUString                      m_sURL;
    OUString                      m_sUserEmail;
    OUString                      m_sFileId;
    OUString                      m_sPath;

    // Members to be set for non-persistent content
    bool                          m_bTransient;
    bool                          m_bIsFolder;
    std::string                   m_sTitle;
    std::string                   m_sMimeType;

    bool isFolder( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );
    
    css::uno::Any getBadArgExcept();

    css::uno::Reference< css::sdbc::XRow >
        getPropertyValues(
            const css::uno::Sequence< css::beans::Property >& rProperties,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    std::shared_ptr<GoogleSession> getSession( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );
    std::shared_ptr<GoogleFile> getGoogleFile( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

private:
    typedef rtl::Reference< Content > ContentRef;
    typedef std::vector< ContentRef > ContentRefList;

    css::uno::Any open(const css::ucb::OpenCommandArgument2& rArg,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    void insert( const css::uno::Reference< css::io::XInputStream > & xInputStream,
        bool bReplaceExisting,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    void transfer( const css::ucb::TransferInfo& rTransferInfo,
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    void destroy( bool bDeletePhysical,
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    void copyData( const css::uno::Reference< css::io::XInputStream >& xIn,
        const css::uno::Reference< css::io::XOutputStream >& xOut );

    css::uno::Sequence< css::uno::Any >
        setPropertyValues( const css::uno::Sequence< css::beans::PropertyValue >& rValues,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    ContentRefList getChildren( );

public:
    Content( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        ContentProvider* pProvider,
        const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier);

    Content( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        ContentProvider* pProvider,
        const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier,
        bool bIsFolder);

    virtual ~Content() override;

    virtual OUString SAL_CALL getImplementationName() override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    virtual OUString SAL_CALL getContentType() override;

    virtual css::uno::Any SAL_CALL execute(
        const css::ucb::Command& aCommand,
        sal_Int32 CommandId,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv ) override;

    virtual void SAL_CALL abort( sal_Int32 CommandId ) override;

    virtual css::uno::Sequence< css::ucb::ContentInfo >
        SAL_CALL queryCreatableContentsInfo() override;

    virtual css::uno::Reference< css::ucb::XContent >
        SAL_CALL createNewContent( const css::ucb::ContentInfo& Info ) override;

    css::uno::Sequence< css::beans::Property > getProperties(
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    OUString getParentURL();

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire()
        noexcept override;
    virtual void SAL_CALL release()
        noexcept override;

    // XTypeProvider
    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() override;
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() override;

    // XServiceInfo
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */