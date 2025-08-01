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

#include <config_folders.h>

#include "util.hxx"

#include <osl/process.h>
#include <osl/file.hxx>
#include <osl/module.hxx>
#include <osl/diagnose.h>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <salhelper/linkhelper.hxx>
#include <salhelper/thread.hxx>
#include <o3tl/environment.hxx>
#include <o3tl/string_view.hxx>
#include <memory>
#include <utility>
#include <algorithm>
#include <map>
#include <string_view>

#if defined(_WIN32)
#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <string.h>

#include "sunjre.hxx"
#include "vendorlist.hxx"
#include "diagnostics.h"
#if defined MACOSX && defined __x86_64__
#include "util_cocoa.hxx"
#endif

using namespace osl;

using ::rtl::Reference;

#ifdef _WIN32
#define HKEY_SUN_JRE L"Software\\JavaSoft\\Java Runtime Environment"
#define HKEY_SUN_SDK L"Software\\JavaSoft\\Java Development Kit"
#endif

#if defined( UNX ) && !defined( MACOSX )
namespace {
char const * const g_arJavaNames[] = {
    "",
    "j2re",
    "j2se",
    "j2sdk",
    "jdk",
    "jre",
    "java"
};

/* These are directory names which could contain multiple java installations.
 */
char const * const g_arCollectDirs[] = {
    "",
#ifndef JVM_ONE_PATH_CHECK
    "j2re/",
    "j2se/",
    "j2sdk/",
    "jdk/",
    "jre/",
    "java/",
#endif
    "jvm/"
};

/* These are directories in which a java installation is
   looked for.
*/
char const * const g_arSearchPaths[] = {
#ifndef JVM_ONE_PATH_CHECK
    "",
    "usr/",
    "usr/local/",
#ifdef X86_64
    "usr/lib64/",
#endif
    "usr/lib/",
    "usr/bin/"
#else
    JVM_ONE_PATH_CHECK
#endif
};
}
#endif //  UNX && !MACOSX

namespace jfw_plugin
{
#if defined(_WIN32)
static bool getSDKInfoFromRegistry(std::vector<OUString> & vecHome);
static bool getJREInfoFromRegistry(std::vector<OUString>& vecJavaHome);
#endif

static bool decodeOutput(std::string_view s, OUString* out);


namespace
{

bool addJREInfo(
    rtl::Reference<VendorBase> const & info,
    std::vector<rtl::Reference<VendorBase>> & infos)
{
    if (std::none_of(infos.begin(), infos.end(), InfoFindSame(info->getHome()))) {
        infos.push_back(info);
        return true;
    } else {
        return false;
    }
}

bool getAndAddJREInfoByPath(
    const OUString& path,
    std::vector<rtl::Reference<VendorBase> > & allInfos,
    std::vector<rtl::Reference<VendorBase> > & addedInfos)
{
    rtl::Reference<VendorBase> aInfo = getJREInfoByPath(path);
    if (aInfo.is()) {
        if (addJREInfo(aInfo, allInfos)) {
            addedInfos.push_back(aInfo);
        }
        return true;
    } else {
        return false;
    }
}

}

namespace {

class FileHandleGuard
{
public:
    explicit FileHandleGuard(oslFileHandle & rHandle):
        m_rHandle(rHandle) {}

    inline ~FileHandleGuard();

    FileHandleGuard(const FileHandleGuard&) = delete;
    FileHandleGuard& operator=(const FileHandleGuard&) = delete;

    oslFileHandle & getHandle() { return m_rHandle; }

private:
    oslFileHandle & m_rHandle;
};

}

inline FileHandleGuard::~FileHandleGuard()
{
    if (m_rHandle != nullptr)
    {
        if (osl_closeFile(m_rHandle) != osl_File_E_None)
        {
            OSL_FAIL("unexpected situation");
        }
    }
}

namespace {

class FileHandleReader
{
public:
    enum Result
    {
        RESULT_OK,
        RESULT_EOF,
        RESULT_ERROR
    };

    explicit FileHandleReader(oslFileHandle & rHandle):
        m_aGuard(rHandle), m_nSize(0), m_nIndex(0), m_bLf(false) {}

    Result readLine(OString * pLine);

private:
    enum { BUFFER_SIZE = 1024 };

    char m_aBuffer[BUFFER_SIZE];
    FileHandleGuard m_aGuard;
    int m_nSize;
    int m_nIndex;
    bool m_bLf;
};

}

FileHandleReader::Result
FileHandleReader::readLine(OString * pLine)
{
    OSL_ENSURE(pLine, "specification violation");

    for (bool bEof = true;; bEof = false)
    {
        if (m_nIndex == m_nSize)
        {
            sal_uInt64 nRead = 0;
            switch (osl_readFile(
                        m_aGuard.getHandle(), m_aBuffer, sizeof(m_aBuffer), &nRead))
            {
            case osl_File_E_PIPE: //HACK! for windows
                nRead = 0;
                [[fallthrough]];
            case osl_File_E_None:
                if (nRead == 0)
                {
                    m_bLf = false;
                    return bEof ? RESULT_EOF : RESULT_OK;
                }
                m_nIndex = 0;
                m_nSize = static_cast< int >(nRead);
                break;
            case osl_File_E_INTR:
                continue;

            default:
                return RESULT_ERROR;
            }
        }

        if (m_bLf && m_aBuffer[m_nIndex] == 0x0A)
            ++m_nIndex;
        m_bLf = false;

        int nStart = m_nIndex;
        while (m_nIndex != m_nSize)
            switch (m_aBuffer[m_nIndex++])
            {
            case 0x0D:
                m_bLf = true;
                [[fallthrough]];
            case 0x0A:
                *pLine += std::string_view(m_aBuffer + nStart,
                                       m_nIndex - 1 - nStart);
                    //TODO! check for overflow, and not very efficient
                return RESULT_OK;
            }

        *pLine += std::string_view(m_aBuffer + nStart, m_nIndex - nStart);
            //TODO! check for overflow, and not very efficient
    }
}

namespace {

class AsynchReader: public salhelper::Thread
{
    size_t  m_nDataSize;
    std::unique_ptr<char[]> m_arData;

    FileHandleGuard m_aGuard;

    virtual ~AsynchReader() override {}

    void execute() override;
public:

    explicit AsynchReader(oslFileHandle & rHandle);

    /** only call this function after this thread has finished.

        That is, call join on this instance and then call getData.

     */
    OString getData();
};

}

AsynchReader::AsynchReader(oslFileHandle & rHandle):
    Thread("jvmfwkAsyncReader"), m_nDataSize(0),
    m_aGuard(rHandle)
{
}

OString AsynchReader::getData()
{
    return OString(m_arData.get(), m_nDataSize);
}

void AsynchReader::execute()
{
    const sal_uInt64 BUFFER_SIZE = 4096;
    char aBuffer[BUFFER_SIZE];
    while (true)
    {
        sal_uInt64 nRead;
        //the function blocks until something could be read or the pipe closed.
        switch (osl_readFile(
                    m_aGuard.getHandle(), aBuffer, BUFFER_SIZE, &nRead))
        {
        case osl_File_E_PIPE: //HACK! for windows
            nRead = 0;
            [[fallthrough]];
        case osl_File_E_None:
            break;
        default:
            return;
        }

        if (nRead == 0)
        {
            break;
        }
        else if (nRead <= BUFFER_SIZE)
        {
            //Save the data we have in m_arData into a temporary array
            std::unique_ptr<char[]> arTmp( new char[m_nDataSize]);
            if (m_nDataSize != 0) {
                memcpy(arTmp.get(), m_arData.get(), m_nDataSize);
            }
            //Enlarge m_arData to hold the newly read data
            m_arData.reset(new char[static_cast<size_t>(m_nDataSize + nRead)]);
            //Copy back the data that was already in m_arData
            memcpy(m_arData.get(), arTmp.get(), m_nDataSize);
            //Add the newly read data to m_arData
            memcpy(m_arData.get() + m_nDataSize, aBuffer, static_cast<size_t>(nRead));
            m_nDataSize += static_cast<size_t>(nRead);
        }
    }
}

bool getJavaProps(const OUString & exePath,
#ifdef JVM_ONE_PATH_CHECK
                  const OUString & homePath,
#endif
                  std::vector<std::pair<OUString, OUString> >& props,
                  bool * bProcessRun)
{
    bool ret = false;

    OSL_ASSERT(!exePath.isEmpty());
    OUString usStartDir;
    //We need to set the CLASSPATH in case the office is started from
    //a different directory. The JREProperties.class is expected to reside
    //next to the plugin, except on macOS where it is in ../Resources/java relative
    //to the plugin.
    OUString sThisLib;
    if (!osl_getModuleURLFromAddress(reinterpret_cast<void *>(&getJavaProps),
                                     & sThisLib.pData))
    {
        return false;
    }
    sThisLib = getDirFromFile(sThisLib);
    OUString sClassPath;
    if (osl_getSystemPathFromFileURL(sThisLib.pData, & sClassPath.pData)
        != osl_File_E_None)
    {
        return false;
    }

#ifdef MACOSX
#if defined __x86_64__
    if (!JvmfwkUtil_isLoadableJVM(exePath))
        return false;
#endif
    if (sClassPath.endsWith("/"))
        sClassPath += "../Resources/java/";
    else
        sClassPath += "/../Resources/java";
#endif

    //prepare the arguments
    sal_Int32 const cArgs = 3;
    OUString arg1 = u"-classpath"_ustr;// + sClassPath;
    OUString arg2 = sClassPath;
    OUString arg3(u"JREProperties"_ustr);
    rtl_uString *args[cArgs] = {arg1.pData, arg2.pData, arg3.pData};

    oslProcess javaProcess= nullptr;
    oslFileHandle fileOut= nullptr;
    oslFileHandle fileErr= nullptr;

    FileHandleReader stdoutReader(fileOut);
    rtl::Reference< AsynchReader > stderrReader(new AsynchReader(fileErr));

    JFW_TRACE2("Executing: " + exePath);
    oslProcessError procErr =
        osl_executeProcess_WithRedirectedIO( exePath.pData,//usExe.pData,
                                             args,
                                             cArgs,                 //sal_uInt32   nArguments,
                                             osl_Process_HIDDEN, //oslProcessOption Options,
                                             nullptr, //oslSecurity Security,
                                             usStartDir.pData,//usStartDir.pData,//usWorkDir.pData, //rtl_uString *strWorkDir,
                                             nullptr, //rtl_uString *strEnvironment[],
                                             0, //  sal_uInt32   nEnvironmentVars,
                                             &javaProcess, //oslProcess *pProcess,
                                             nullptr,//oslFileHandle *pChildInputWrite,
                                             &fileOut,//oslFileHandle *pChildOutputRead,
                                             &fileErr);//oslFileHandle *pChildErrorRead);

    if( procErr != osl_Process_E_None)
    {
        JFW_TRACE2("Execution failed");
        *bProcessRun = false;
        SAL_WARN("jfw",
            "osl_executeProcess failed (" << ret << "): \"" << exePath << "\"");
        return ret;
    }
    else
    {
        JFW_TRACE2("Java executed successfully");
        *bProcessRun = true;
    }

    //Start asynchronous reading (different thread) of error stream
    stderrReader->launch();

    //Use this thread to read output stream
    FileHandleReader::Result rs = FileHandleReader::RESULT_OK;
    JFW_TRACE2("Properties found:");
    while (true)
    {
        OString aLine;
        rs = stdoutReader.readLine( & aLine);
        if (rs != FileHandleReader::RESULT_OK)
            break;
        OUString sLine;
        if (!decodeOutput(aLine, &sLine))
            continue;
        JFW_TRACE2("  \"" << sLine << "\"");
        sLine = sLine.trim();
        if (sLine.isEmpty())
            continue;
        //The JREProperties class writes key value pairs, separated by '='
        sal_Int32 index = sLine.indexOf('=');
        OSL_ASSERT(index != -1);
        OUString sKey = sLine.copy(0, index);
        OUString sVal = sLine.copy(index + 1);

#ifdef JVM_ONE_PATH_CHECK
        //replace absolute path by linux distro link
        OUString sHomeProperty("java.home");
        if(sHomeProperty.equals(sKey))
        {
            sVal = homePath + "/jre";
        }
#endif

        props.emplace_back(sKey, sVal);
    }

    if (rs != FileHandleReader::RESULT_ERROR && !props.empty())
        ret = true;

    //process error stream data
    stderrReader->join();
    JFW_TRACE2("Java wrote to stderr:\" "
               << stderrReader->getData() << " \"");

    TimeValue waitMax= {5 ,0};
    procErr = osl_joinProcessWithTimeout(javaProcess, &waitMax);
    OSL_ASSERT(procErr == osl_Process_E_None);
    osl_freeProcessHandle(javaProcess);
    return ret;
}

/* converts the properties printed by JREProperties.class into
    readable strings. The strings are encoded as integer values separated
    by spaces.
 */
bool decodeOutput(std::string_view s, OUString* out)
{
    OSL_ASSERT(out != nullptr);
    OUStringBuffer buff(512);
    sal_Int32 nIndex = 0;
    do
    {
        std::string_view aToken = o3tl::getToken(s, 0, ' ', nIndex );
        if (!aToken.empty())
        {
            for (size_t i = 0; i < aToken.size(); ++i)
            {
                if (aToken[i] < '0' || aToken[i] > '9')
                    return false;
            }
            sal_Unicode value = static_cast<sal_Unicode>(o3tl::toInt32(aToken));
            buff.append(value);
        }
    } while (nIndex >= 0);

    *out = buff.makeStringAndClear();
    return true;
}


#if defined(_WIN32)

static bool getJavaInfoFromRegistry(const wchar_t* szRegKey,
                             std::vector<OUString>& vecJavaHome)
{
    HKEY    hRoot;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_ENUMERATE_SUB_KEYS, &hRoot)
        == ERROR_SUCCESS)
    {
        DWORD dwIndex = 0;
        const DWORD BUFFSIZE = 1024;
        wchar_t bufVersion[BUFFSIZE];
        FILETIME fileTime;
        DWORD nNameLen = sizeof(bufVersion);

        // Iterate over all subkeys of HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment
        while (RegEnumKeyExW(hRoot, dwIndex, bufVersion, &nNameLen, nullptr, nullptr, nullptr, &fileTime) != ERROR_NO_MORE_ITEMS)
        {
            HKEY    hKey;
            // Open a Java Runtime Environment sub key, e.g. "1.4.0"
            if (RegOpenKeyExW(hRoot, bufVersion, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
            {
                DWORD   dwType;
                DWORD   dwTmpPathLen= 0;
                // Get the path to the JavaHome every JRE entry
                // Find out how long the string for JavaHome is and allocate memory to hold the path
                if( RegQueryValueExW(hKey, L"JavaHome", nullptr, &dwType, nullptr, &dwTmpPathLen)== ERROR_SUCCESS)
                {
                    unsigned char* szTmpPath= static_cast<unsigned char *>(malloc(dwTmpPathLen+sizeof(sal_Unicode)));
                    assert(szTmpPath && "Don't handle OOM conditions");
                    // According to https://msdn.microsoft.com/en-us/ms724911, the application should ensure
                    // that the string is properly terminated before using it
                    for (DWORD i = 0; i < sizeof(sal_Unicode); ++i)
                        szTmpPath[dwTmpPathLen + i] = 0;
                    // Get the path for the runtime lib
                    if(RegQueryValueExW(hKey, L"JavaHome", nullptr, &dwType, szTmpPath, &dwTmpPathLen) == ERROR_SUCCESS)
                    {
                        // There can be several version entries referring with the same JavaHome,e.g 1.4 and 1.4.1
                        OUString usHome(reinterpret_cast<sal_Unicode*>(szTmpPath));
                        // check if there is already an entry with the same JavaHomeruntime lib
                        // if so, we use the one with the more accurate version
                        OUString usHomeUrl;
                        if (osl_getFileURLFromSystemPath(usHome.pData, & usHomeUrl.pData) ==
                            osl_File_E_None)
                        {
                            bool bAppend= true;
                            //iterate over the vector with java home strings
                            for (auto const& javaHome : vecJavaHome)
                            {
                                if(usHomeUrl.equals(javaHome))
                                {
                                    bAppend= false;
                                    break;
                                }
                            }
                            // Save the home dir
                            if(bAppend)
                            {
                                vecJavaHome.push_back(usHomeUrl);
                            }
                        }
                    }
                    free( szTmpPath);
                    RegCloseKey(hKey);
                }
            }
            dwIndex ++;
            nNameLen = BUFFSIZE;
        }
        RegCloseKey(hRoot);
    }
    return true;
}


bool getSDKInfoFromRegistry(std::vector<OUString> & vecHome)
{
    return getJavaInfoFromRegistry(HKEY_SUN_SDK, vecHome);
}

bool getJREInfoFromRegistry(std::vector<OUString>& vecJavaHome)
{
    return getJavaInfoFromRegistry(HKEY_SUN_JRE, vecJavaHome);
}

static void addJavaInfoFromWinReg(
    std::vector<rtl::Reference<VendorBase> > & allInfos,
    std::vector<rtl::Reference<VendorBase> > & addedInfos)
{
        // Get Java s from registry
    std::vector<OUString> vecJavaHome;
    if(getSDKInfoFromRegistry(vecJavaHome))
    {
        // create impl objects
        for (auto const& javaHome : vecJavaHome)
        {
            getAndAddJREInfoByPath(javaHome, allInfos, addedInfos);
        }
    }

    vecJavaHome.clear();
    if(getJREInfoFromRegistry(vecJavaHome))
    {
        for (auto const& javaHome : vecJavaHome)
        {
            getAndAddJREInfoByPath(javaHome, allInfos, addedInfos);
        }
    }

    vecJavaHome.clear();
    if (getJavaInfoFromRegistry(L"Software\\JavaSoft\\JDK", vecJavaHome)) {
        for (auto const & javaHome: vecJavaHome) {
            getAndAddJREInfoByPath(javaHome, allInfos, addedInfos);
        }
    }

    vecJavaHome.clear();
    if (getJavaInfoFromRegistry(L"Software\\JavaSoft\\JRE", vecJavaHome)) {
        for (auto const & javaHome: vecJavaHome) {
            getAndAddJREInfoByPath(javaHome, allInfos, addedInfos);
        }
    }
}

#endif // _WIN32

void bubbleSortVersion(std::vector<rtl::Reference<VendorBase> >& vec)
{
    if(vec.empty())
        return;
    int size= vec.size() - 1;
    int cIter= 0;
    // sort for version
    for(int i= 0; i < size; i++)
    {
        for(int j= size; j > 0 + cIter; j--)
        {
            rtl::Reference<VendorBase>& cur= vec.at(j);
            rtl::Reference<VendorBase>& next= vec.at(j-1);

            int nCmp = 0;
            // comparing invalid SunVersion s is possible, they will be less than a
            // valid version

            //check if version of current is recognized, by comparing it with itself
            try
            {
                (void)cur->compareVersions(cur->getVersion());
            }
            catch (MalformedVersionException &)
            {
                nCmp = -1; // current < next
            }
            //The version of cur is valid, now compare with the second version
            if (nCmp == 0)
            {
                try
                {
                    nCmp = cur->compareVersions(next->getVersion());
                }
                catch (MalformedVersionException & )
                {
                    //The second version is invalid, therefore it regards less.
                    nCmp = 1;
                }
            }
            if(nCmp == 1) // cur > next
            {
                std::swap(cur, next);
            }
        }
        ++cIter;
    }
}


void addJREInfoFromBinPath(
    const OUString& path, std::vector<rtl::Reference<VendorBase>> & allInfos,
    std::vector<rtl::Reference<VendorBase>> & addedInfos)
{
    // file:///c:/jre/bin
    //map:       jre/bin/java.exe

    for ( sal_Int32 pos = 0;
          gVendorMap[pos].sVendorName != nullptr; ++pos )
    {
        std::vector<OUString> vecPaths;
        getJavaExePaths_func pFunc = gVendorMap[pos].getJavaFunc;

        int size = 0;
        char const* const* arExePaths = (*pFunc)(&size);
        vecPaths = getVectorFromCharArray(arExePaths, size);

        //make sure argument path does not end with '/'
        OUString sBinPath = path;
        if (path.endsWith("/"))
            sBinPath = path.copy(0, path.getLength() - 1);

        for (auto const& looppath : vecPaths)
        {
            //the map contains e.g. jre/bin/java.exe
            //get the directory where the executable is contained
            OUString sHome;
            sal_Int32 index = looppath.lastIndexOf('/');
            if (index == -1)
            {
                //map contained only : "java.exe, then the argument
                //path is already the home directory
                sHome = sBinPath;
            }
            else
            {
                // jre/bin/jre -> jre/bin
                OUString sMapPath = looppath.copy(0, index);
                index = sBinPath.lastIndexOf(sMapPath);
                if (index != -1
                    && (index + sMapPath.getLength() == sBinPath.getLength())
                    && sBinPath[index - 1] == '/')
                {
                    sHome = sBinPath.copy(index - 1);
                }
            }
            if (!sHome.isEmpty()
                && getAndAddJREInfoByPath(path, allInfos, addedInfos))
            {
                return;
            }
        }
    }
}

std::vector<Reference<VendorBase> > addAllJREInfos(
    bool checkJavaHomeAndPath,
    std::vector<rtl::Reference<VendorBase>> & allInfos)
{
    std::vector<Reference<VendorBase> > addedInfos;

#if defined(_WIN32)
    // Get Javas from the registry
    addJavaInfoFromWinReg(allInfos, addedInfos);
#endif // _WIN32

    if (checkJavaHomeAndPath) {
        addJavaInfoFromJavaHome(allInfos, addedInfos);
        //this function should be called after addJavaInfosDirScan.
        //Otherwise in SDKs Java may be started twice
        addJavaInfosFromPath(allInfos, addedInfos);
    }

#ifdef UNX
    addJavaInfosDirScan(allInfos, addedInfos);
#endif

    bubbleSortVersion(addedInfos);
    return addedInfos;
}


std::vector<OUString> getVectorFromCharArray(char const * const * ar, int size)
{
    std::vector<OUString> vec;
    for( int i = 0; i < size; i++)
    {
        OUString s(ar[i], strlen(ar[i]), RTL_TEXTENCODING_UTF8);
        vec.push_back(s);
    }
    return vec;
}

/** Checks if the path is a directory. Links are resolved.
    In case of an error the returned string has the length 0.
    Otherwise the returned string is the "resolved" file URL.
 */
static OUString resolveDirPath(const OUString & path)
{
    OUString ret;
    salhelper::LinkResolver aResolver(osl_FileStatus_Mask_Type |
                                       osl_FileStatus_Mask_FileURL);
    if (aResolver.fetchFileStatus(path) == osl::FileBase::E_None)
    {
        //check if this is a directory
        if (aResolver.m_aStatus.getFileType() == FileStatus::Directory)
        {
#ifndef JVM_ONE_PATH_CHECK
            ret = aResolver.m_aStatus.getFileURL();
#else
            ret = path;
#endif
        }
    }
    return ret;
}
/** Checks if the path is a file. If it is a link to a file than
    it is resolved.
 */
static OUString resolveFilePath(const OUString & path)
{
    OUString ret;
    salhelper::LinkResolver aResolver(osl_FileStatus_Mask_Type |
                                       osl_FileStatus_Mask_FileURL);
    if (aResolver.fetchFileStatus(path) == osl::FileBase::E_None)
    {
        //check if this is a file
        if (aResolver.m_aStatus.getFileType() == FileStatus::Regular)
        {
#ifndef JVM_ONE_PATH_CHECK
            ret = aResolver.m_aStatus.getFileURL();
#else
            ret = path;
#endif
        }
    }
    return ret;
}

rtl::Reference<VendorBase> getJREInfoByPath(
    const OUString& path)
{
    rtl::Reference<VendorBase> ret;
    static std::vector<OUString> vecBadPaths;

    static std::map<OUString, rtl::Reference<VendorBase> > mapJREs;
    OUString sFilePath;
    std::vector<std::pair<OUString, OUString> > props;

    OUString sResolvedDir = resolveDirPath(path);
    // If this path is invalid then there is no chance to find a JRE here
    if (sResolvedDir.isEmpty())
    {
        return nullptr;
    }

    //check if the directory path is good, that is a JRE was already recognized.
    //Then we need not detect it again
    //For example, a sun JDK contains <jdk>/bin/java and <jdk>/jre/bin/java.
    //When <jdk>/bin/java has been found then we need not find <jdk>/jre/bin/java.
    //Otherwise we would execute java two times for every JDK found.
    auto entry2 = find_if(mapJREs.cbegin(), mapJREs.cend(),
                           SameOrSubDirJREMap(sResolvedDir));
    if (entry2 != mapJREs.end())
    {
        JFW_TRACE2("JRE found again (detected before): " << sResolvedDir);
        return entry2->second;
    }

    for ( sal_Int32 pos = 0;
          gVendorMap[pos].sVendorName != nullptr; ++pos )
    {
        std::vector<OUString> vecPaths;
        getJavaExePaths_func pFunc = gVendorMap[pos].getJavaFunc;

        int size = 0;
        char const* const* arExePaths = (*pFunc)(&size);
        vecPaths = getVectorFromCharArray(arExePaths, size);

        bool bBreak = false;
        for (auto const& looppath : vecPaths)
        {
            //if the path is a link, then resolve it
            //check if the executable exists at all

            //path can be only "file:///". Then do not append a '/'
            //sizeof counts the terminating 0
            OUString sFullPath;
            if (path.getLength() == sizeof("file:///") - 1)
                sFullPath = sResolvedDir + looppath;
            else
                sFullPath = sResolvedDir + "/" + looppath;

            sFilePath = resolveFilePath(sFullPath);

            if (sFilePath.isEmpty())
            {
                //The file path (to java exe) is not valid
                auto ifull = find(vecBadPaths.cbegin(), vecBadPaths.cend(), sFullPath);
                if (ifull == vecBadPaths.cend())
                {
                    vecBadPaths.push_back(sFullPath);
                }
                continue;
            }

            auto ifile = find(vecBadPaths.cbegin(), vecBadPaths.cend(), sFilePath);
            if (ifile != vecBadPaths.cend())
            {
                continue;
            }

            auto entry =  mapJREs.find(sFilePath);
            if (entry != mapJREs.end())
            {
                JFW_TRACE2("JRE found again (detected before): " << sFilePath);

                return entry->second;
            }

            bool bProcessRun= false;
            if (!getJavaProps(sFilePath,
#ifdef JVM_ONE_PATH_CHECK
                             sResolvedDir,
#endif
                             props, & bProcessRun))
            {
                //The java executable could not be run or the system properties
                //could not be retrieved. We can assume that this java is corrupt.
                vecBadPaths.push_back(sFilePath);
                //If there was a java executable, that could be run but we did not get
                //the system properties, then we also assume that the whole Java installation
                //does not work. In a jdk there are two executables. One in jdk/bin and the other
                //in jdk/jre/bin. We do not search any further, because we assume that if one java
                //does not work then the other does not work as well. This saves us to run java
                //again which is quite costly.
                if (bProcessRun)
                {
                    // 1.3.1 special treatment: jdk/bin/java and /jdk/jre/bin/java are links to
                    //a script, named .java_wrapper. The script starts jdk/bin/sparc/native_threads/java
                    //or jdk/jre/bin/sparc/native_threads/java. The script uses the name with which it was
                    //invoked to build the path to the executable. It we start the script directly as .java_wrapper
                    //then it tries to start a jdk/.../native_threads/.java_wrapper. Therefore the link, which
                    //is named java, must be used to start the script.
                    getJavaProps(sFullPath,
#ifdef JVM_ONE_PATH_CHECK
                                 sResolvedDir,
#endif
                                 props, & bProcessRun);
                    // Either we found a working 1.3.1
                    // Or the java is broken. In both cases we stop searching under this "root" directory
                    bBreak = true;
                    break;
                }
                //sFilePath is no working java executable. We continue with another possible
                //path.
                else
                {
                    continue;
                }
            }
            //sFilePath is a java and we could get the system properties. We proceed with this
            //java.
            else
            {
                bBreak = true;
                break;
            }
        }
        if (bBreak)
            break;
    }

    if (props.empty())
    {
        return rtl::Reference<VendorBase>();
    }

    //find java.vendor property
    OUString sVendorName;

    for (auto const& prop : props)
    {
        if (prop.first == "java.vendor")
        {
            sVendorName = prop.second;
            break;
        }
    }

    auto knownVendor = false;
    if (!sVendorName.isEmpty())
    {
        //find the creator func for the respective vendor name
        for ( sal_Int32 c = 0;
              gVendorMap[c].sVendorName != nullptr; ++c )
        {
            OUString sNameMap(gVendorMap[c].sVendorName, strlen(gVendorMap[c].sVendorName),
                              RTL_TEXTENCODING_ASCII_US);
            if (sNameMap == sVendorName)
            {
                ret = createInstance(gVendorMap[c].createFunc, props);
                knownVendor = true;
                break;
            }
        }
    }
    // For unknown vendors, try SunInfo as fallback:
    if (!knownVendor)
    {
        ret = createInstance(SunInfo::createInstance, props);
    }
    if (!ret.is())
    {
        vecBadPaths.push_back(sFilePath);
    }
    else
    {
        JFW_TRACE2("Found JRE: " << sResolvedDir << " at: " << path);

        mapJREs.emplace(sResolvedDir, ret);
        mapJREs.emplace(sFilePath, ret);
    }

    return ret;
}

Reference<VendorBase> createInstance(createInstance_func pFunc,
                                     const std::vector<std::pair<OUString, OUString> >& properties)
{

    Reference<VendorBase> aBase = (*pFunc)();
    if (aBase.is())
    {
        if (!aBase->initialize(properties))
            aBase = nullptr;
    }
    return aBase;
}

inline OUString getDirFromFile(std::u16string_view usFilePath)
{
    size_t index = usFilePath.rfind('/');
    return OUString(usFilePath.substr(0, index));
}

void addJavaInfosFromPath(
    std::vector<rtl::Reference<VendorBase>> & allInfos,
    std::vector<rtl::Reference<VendorBase>> & addedInfos)
{
#if !defined JVM_ONE_PATH_CHECK
// Get Java from PATH environment variable
    OUString usAllPath = o3tl::getEnvironment(u"PATH"_ustr);
    if (usAllPath.isEmpty())
        return;

    sal_Int32 nIndex = 0;
    do
    {
        OUString usToken( usAllPath.getToken( 0, SAL_PATHSEPARATOR, nIndex ) );
        OUString usTokenUrl;
        if(File::getFileURLFromSystemPath(usToken, usTokenUrl) == File::E_None)
        {
            if(!usTokenUrl.isEmpty())
            {
                OUString usBin;
                if(usTokenUrl == ".")
                {
                    OUString usWorkDirUrl;
                    if(osl_Process_E_None == osl_getProcessWorkingDir(&usWorkDirUrl.pData))
                        usBin= usWorkDirUrl;
                }
                else if(usTokenUrl == "..")
                {
                    OUString usWorkDir;
                    if(osl_Process_E_None == osl_getProcessWorkingDir(&usWorkDir.pData))
                        usBin= getDirFromFile(usWorkDir);
                }
                else
                {
                    usBin = usTokenUrl;
                }
                if(!usBin.isEmpty())
                {
                    addJREInfoFromBinPath(usBin, allInfos, addedInfos);
                }
            }
        }
    }
    while ( nIndex >= 0 );
#endif
}


void addJavaInfoFromJavaHome(
    std::vector<rtl::Reference<VendorBase>> & allInfos,
    std::vector<rtl::Reference<VendorBase>> & addedInfos)
{
#if !defined JVM_ONE_PATH_CHECK
    // Get Java from JAVA_HOME environment

    // Note that on macOS is it not normal at all to have a JAVA_HOME environment
    // variable. We set it in our build environment for build-time programs, though,
    // so it is set when running unit tests that involve Java functionality. (Which affects
    // at least CppunitTest_dbaccess_dialog_save, too, and not only the JunitTest ones.)
    OUString sHome = o3tl::getEnvironment(u"JAVA_HOME"_ustr);
    if (!sHome.isEmpty())
    {
        OUString sHomeUrl;
        if(File::getFileURLFromSystemPath(sHome, sHomeUrl) == File::E_None)
        {
            getAndAddJREInfoByPath(sHomeUrl, allInfos, addedInfos);
        }
    }
#endif
}

bool makeDriveLetterSame(OUString * fileURL)
{
    bool ret = false;
    DirectoryItem item;
    if (DirectoryItem::get(*fileURL, item) == File::E_None)
    {
        FileStatus status(osl_FileStatus_Mask_FileURL);
        if (item.getFileStatus(status) == File::E_None)
        {
            *fileURL = status.getFileURL();
            ret = true;
        }
    }
    return ret;
}

#ifdef UNX
#ifdef __sun

void addJavaInfosDirScan(
    std::vector<rtl::Reference<VendorBase>> & allInfos,
    std::vector<rtl::Reference<VendorBase>> & addedInfos)
{
    JFW_TRACE2("Checking /usr/jdk/latest");
    getAndAddJREInfoByPath("file:////usr/jdk/latest", allInfos, addedInfos);
}

#else
void addJavaInfosDirScan(
    std::vector<rtl::Reference<VendorBase>> & allInfos,
    std::vector<rtl::Reference<VendorBase>> & addedInfos)
{
#ifdef MACOSX
    // Ignore all but Oracle's JDK as loading Apple's Java and Oracle's JRE
    // will cause macOS's JavaVM framework to display a dialog and invoke
    // exit() when loaded via JNI on macOS 10.10
    Directory aDir("file:///Library/Java/JavaVirtualMachines");
    if (aDir.open() == File::E_None)
    {
        DirectoryItem aItem;
        while (aDir.getNextItem(aItem) == File::E_None)
        {
            FileStatus aStatus(osl_FileStatus_Mask_FileURL);
            if (aItem.getFileStatus(aStatus) == File::E_None)
            {
                OUString aItemURL( aStatus.getFileURL() );
                if (aItemURL.getLength())
                {
                    aItemURL += "/Contents/Home";
                    if (DirectoryItem::get(aItemURL, aItem) == File::E_None)
                        getAndAddJREInfoByPath(aItemURL, allInfos, addedInfos);
                }
            }
        }
        aDir.close();
    }
#else // MACOSX
    OUString excMessage = u"[Java framework] sunjavaplugin: "
                          "Error in function addJavaInfosDirScan in util.cxx."_ustr;
    int cJavaNames= SAL_N_ELEMENTS(g_arJavaNames);
    std::unique_ptr<OUString[]> sarJavaNames(new OUString[cJavaNames]);
    OUString *arNames = sarJavaNames.get();
    for(int i= 0; i < cJavaNames; i++)
        arNames[i] = OUString(g_arJavaNames[i], strlen(g_arJavaNames[i]),
                              RTL_TEXTENCODING_UTF8);

    int cSearchPaths= SAL_N_ELEMENTS(g_arSearchPaths);
    std::unique_ptr<OUString[]> sarPathNames(new OUString[cSearchPaths]);
    OUString *arPaths = sarPathNames.get();
    for(int c = 0; c < cSearchPaths; c++)
        arPaths[c] = OUString(g_arSearchPaths[c], strlen(g_arSearchPaths[c]),
                               RTL_TEXTENCODING_UTF8);

    int cCollectDirs = SAL_N_ELEMENTS(g_arCollectDirs);
    std::unique_ptr<OUString[]> sarCollectDirs(new OUString[cCollectDirs]);
    OUString *arCollectDirs = sarCollectDirs.get();
    for(int d = 0; d < cCollectDirs; d++)
        arCollectDirs[d] = OUString(g_arCollectDirs[d], strlen(g_arCollectDirs[d]),
                               RTL_TEXTENCODING_UTF8);


    for( int ii = 0; ii < cSearchPaths; ii ++)
    {
        OUString usDir1("file:///" + arPaths[ii]);
        DirectoryItem item;
        if(DirectoryItem::get(usDir1, item) == File::E_None)
        {
            for(int j= 0; j < cCollectDirs; j++)
            {
                OUString usDir2(usDir1 + arCollectDirs[j]);
                // prevent that we scan the whole /usr, /usr/lib, etc directories
                if (!arCollectDirs[j].isEmpty())
                {
                    //usr/java/xxx
                    //Examining every subdirectory
                    Directory aCollectionDir(usDir2);

                    Directory::RC openErr = aCollectionDir.open();
                    switch (openErr)
                    {
                    case File::E_None:
                        break;
                    case File::E_NOENT:
                    case File::E_NOTDIR:
                        continue;
                    case File::E_ACCES:
                        JFW_TRACE2("Could not read directory " << usDir2 << " because of missing access rights");
                        continue;
                    default:
                        JFW_TRACE2("Could not read directory " << usDir2 << ". Osl file error: " << openErr);
                        continue;
                    }

                    DirectoryItem curIt;
                    File::RC errNext = File::E_None;
                    while( (errNext = aCollectionDir.getNextItem(curIt)) == File::E_None)
                    {
                        FileStatus aStatus(osl_FileStatus_Mask_FileURL);
                        File::RC errStatus = File::E_None;
                        if ((errStatus = curIt.getFileStatus(aStatus)) != File::E_None)
                        {
                            JFW_TRACE2(excMessage + "getFileStatus failed with error " << errStatus);
                            continue;
                        }
                        JFW_TRACE2("Checking if directory: " << aStatus.getFileURL() << " is a Java");

                        getAndAddJREInfoByPath(
                            aStatus.getFileURL(), allInfos, addedInfos);
                    }

                    JFW_ENSURE(errNext == File::E_None || errNext == File::E_NOENT,
                                "[Java framework] sunjavaplugin: "
                                "Error while iterating over contents of "
                                + usDir2 + ". Osl file error: "
                                + OUString::number(openErr));
                }
                else
                {
                    //usr/java
                    //When we look directly into a dir like /usr, /usr/lib, etc. then we only
                    //look for certain java directories, such as jre, jdk, etc. We do not want
                    //to examine the whole directory because of performance reasons.
                    DirectoryItem item2;
                    if(DirectoryItem::get(usDir2, item2) == File::E_None)
                    {
                        for( int k= 0; k < cJavaNames; k++)
                        {
                            // /usr/java/j2re1.4.0
                            OUString usDir3(usDir2 + arNames[k]);

                            DirectoryItem item3;
                            if(DirectoryItem::get(usDir3, item3) == File::E_None)
                            {
                                //remove trailing '/'
                                sal_Int32 islash = usDir3.lastIndexOf('/');
                                if (islash == usDir3.getLength() - 1
                                    && (islash
                                        > RTL_CONSTASCII_LENGTH("file://")))
                                    usDir3 = usDir3.copy(0, islash);
                                getAndAddJREInfoByPath(
                                    usDir3, allInfos, addedInfos);
                            }
                        }
                    }
                }
            }
        }
    }
#endif // MACOSX
}
#endif // ifdef __sun
#endif // ifdef UNX
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
