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
#ifndef INCLUDED_SW_INC_VIEW_HXX
#define INCLUDED_SW_INC_VIEW_HXX

#include <vcl/timer.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/objsh.hxx>
#include <editeng/svxenum.hxx>
#include <sfx2/zoomitem.hxx>
#include <svx/ruler.hxx>
#include <svx/fmshell.hxx>
#include <svx/svdobj.hxx>
#include <svl/style.hxx>
#include "swdllapi.h"
#include "swtypes.hxx"
#include "shellid.hxx"
#include "viewsh.hxx"
#include "names.hxx"

#include <svx/sdr/overlay/overlayobject.hxx>

class SwTextFormatColl;
class SwPageDesc;
class SwFrameFormat;
class SwCharFormat;
class SwNumRule;
class SwGlossaryHdl;
class SwDrawBase;
class SvxLRSpaceItem;
class SwDocShell;
class SwScrollbar;
class SvBorder;
class Ruler;
class SvxSearchItem;
class SearchAttrItemList;
class SvxSearchDialog;
class SdrPageView;
class SwEditWin;
class SwWrtShell;
class SwView_Impl;
struct SwSearchOptions;
class CommandEvent;
class InsCaptionOpt;
class SvGlobalName;
class SwTransferable;
class SwMailMergeConfigItem;
class SwTextNode; // #i23726#
class SwFormatClipboard;
struct SwConversionArgs;
class GraphicFilter;
class SwPostItMgr;
enum class SotExchangeDest;
enum class SvxSearchCmd;
enum class SelectionType : sal_Int32;
class SwNode;
class SwMarkName;

namespace com::sun::star::view { class XSelectionSupplier; }
namespace sfx2 { class FileDialogHelper; class DocumentTimer; }
namespace weld { class Scrollbar; }

const tools::Long nLeftOfst = -370;
const tools::Long nScrollX  =   30;
const tools::Long nScrollY  =   30;

#define MINZOOM 20
#define MAXZOOM 600

#define MAX_MARKS 5

enum class ShellMode
{
    Text,
    Frame,
    Graphic,
    Object,
    Draw,
    DrawForm,
    DrawText,
    Bezier,
    ListText,
    TableText,
    TableListText,
    Media,
    ExtrudedCustomShape,
    FontWork,
    PostIt
};

// apply a template
struct SwApplyTemplate
{
    union
    {
        SwTextFormatColl* pTextColl;
        SwPageDesc*   pPageDesc;
        SwFrameFormat*     pFrameFormat;
        SwCharFormat*    pCharFormat;
        SwNumRule*    pNumRule;
    } aColl;

    SfxStyleFamily eType;
    sal_uInt16 nColor;
    SwFormatClipboard* m_pFormatClipboard;
    size_t nUndo;     //< The initial undo stack depth.

    SwApplyTemplate() :
        eType(SfxStyleFamily::None),
        nColor(0),
        m_pFormatClipboard(nullptr),
        nUndo(0)
    {
        aColl.pTextColl = nullptr;
    }
};

class SwView;

// manage connection and disconnection of SwView and SwDocShell
class SwViewGlueDocShell
{
private:
    SwView& m_rView;
public:
    SwViewGlueDocShell(SwView& rView, SwDocShell& rDocSh);
    ~SwViewGlueDocShell();
};

// view of a document
class SW_DLLPUBLIC SwView: public SfxViewShell
{
    friend class SwHHCWrapper;
    friend class SwHyphWrapper;
    friend class SwView_Impl;
    friend class SwClipboardChangeListener;

    // selection cycle
    struct SelectCycle
    {
        Point m_pInitialCursor;
        Point m_MarkPt;
        Point m_PointPt;
        sal_uInt16 nStep;

        SelectCycle() :
            nStep(0) {}
    };

    // search & replace
    static SvxSearchItem           *s_pSrchItem;

    static sal_uInt16       s_nMoveType; // for buttons below the scrollbar (viewmdi)
    static sal_Int32        s_nActMark; // current jump mark for unknown mark

    static bool             s_bExtra;
    static bool             s_bFound;
    static bool             s_bJustOpened;

    static std::unique_ptr<SearchAttrItemList> s_xSearchList;
    static std::unique_ptr<SearchAttrItemList> s_xReplaceList;

    Timer               m_aTimer;         // for delayed ChgLnks during an action
    OUString            m_sSwViewData,
    //and the new cursor position if the user double click in the PagePreview
                        m_sNewCursorPos;
    // to support keyboard the number of the page to go to can be set too
    sal_uInt16              m_nNewPage;

    sal_uInt16          m_nOldPageNum;
    UIName              m_sOldSectionName;

    Point               m_aTabColFromDocPos;  // moving table columns out of the document
    SwTextNode           * m_pNumRuleNodeFromDoc; // Moving indent of numrule #i23726#

    Size                m_aDocSz;         // current document size
    tools::Rectangle           m_aVisArea;       // visible region

    VclPtr<SwEditWin>    m_pEditWin;
    std::unique_ptr<SwWrtShell> m_pWrtShell;
    std::unique_ptr<SwViewGlueDocShell> m_xGlueDocShell;

    SfxShell            *m_pShell;        // current SubShell at the dispatcher
    FmFormShell         *m_pFormShell;    // DB-FormShell

    std::unique_ptr<SwView_Impl> m_pViewImpl;     // Impl-data for UNO + Basic

    VclPtr<SwScrollbar>  m_pHScrollbar,   // MDI control elements
                         m_pVScrollbar;

    bool                m_bHScrollbarEnabled;
    bool                m_bVScrollbarEnabled;

    VclPtr<SvxRuler>    m_pHRuler,
                        m_pVRuler;

    std::unique_ptr<SwGlossaryHdl> m_pGlosHdl;          // handle text block
    std::unique_ptr<SwDrawBase>    m_pDrawActual;

    const SwFrameFormat      *m_pLastTableFormat;
    const SwFrameFormat* m_pLastFlyFormat;

    std::unique_ptr<SwFormatClipboard> m_pFormatClipboard; //holds data for format paintbrush

    std::unique_ptr<SwPostItMgr> m_pPostItMgr;
    
    std::unique_ptr<sfx2::DocumentTimer> m_pDocumentTimer;

    SelectionType       m_nSelectionType;
    sal_uInt16          m_nPageCnt;

    // current draw mode
    sal_uInt16          m_nDrawSfxId;
    OUString            m_sDrawCustom; //some drawing types are marked with strings!
    sal_uInt16          m_nFormSfxId;
    SdrObjKind          m_eFormObjKind;
    SotExchangeDest     m_nLastPasteDestination;

    // save the border distance status from SwView::StateTabWin to re-use it in SwView::ExecTabWin()
    sal_uInt16          m_nLeftBorderDistance;
    sal_uInt16          m_nRightBorderDistance;

    SvxSearchCmd        m_eLastSearchCommand;

    bool m_bWheelScrollInProgress;
    double          m_fLastZoomScale = 0;
    double          m_fAccumulatedZoom = 0;

    bool            m_bCenterCursor : 1,
                    m_bTopCursor : 1,
                    m_bTabColFromDoc : 1,
                    m_bTabRowFromDoc : 1,
                    m_bSetTabColFromDoc : 1 ,
                    m_bSetTabRowFromDoc : 1,
                    m_bAttrChgNotified : 1,
                    m_bAttrChgNotifiedWithRegistrations : 1,
                    m_bVerbsActive : 1,
                    m_bDrawRotate : 1,
                    m_bDrawSelMode : 1,
                    m_bShowAtResize : 1,
                    m_bInOuterResizePixel : 1,
                    m_bInInnerResizePixel : 1,
                    m_bPasteState : 1,
                    m_bPasteSpecialState : 1,
                    m_bInMailMerge : 1,
                    m_bInDtor : 1, //detect destructor to prevent creating of sub shells while closing
                    m_bOldShellWasPagePreview : 1,
                    m_bIsPreviewDoubleClick : 1, // #i114045#
                    m_bMakeSelectionVisible : 1, // transport the bookmark selection
                    m_bForceChangesToolbar : 1;  // on load of documents with change tracking
    bool m_bInitOnceCompleted = false;

    /// LibreOfficeKit has to force the page size for PgUp/PgDown
    /// functionality based on the user's view, instead of using the m_aVisArea.
    SwTwips         m_nLOKPageUpDownOffset;

    SelectCycle m_aSelectCycle;

    int m_nMaxOutlineLevelShown = 10;

    bool m_bIsHighlightCharDF = false;
    bool m_bIsSpotlightParaStyles = false;
    bool m_bIsSpotlightCharStyles = false;

    bool m_bDying = false;

    static constexpr sal_uInt16 MAX_ZOOM_PERCENT = 600;
    static constexpr sal_uInt16 MIN_ZOOM_PERCENT = 20;

    // methods for searching
    // set search context
    SAL_DLLPRIVATE bool          SearchAndWrap(bool bApi);
    SAL_DLLPRIVATE sal_Int32 SearchAll();
    SAL_DLLPRIVATE sal_Int32     FUNC_Search( const SwSearchOptions& rOptions );
    SAL_DLLPRIVATE void          Replace();

    bool                        IsDocumentBorder();

    SAL_DLLPRIVATE bool          IsTextTool() const;

    DECL_DLLPRIVATE_LINK( TimeoutHdl, Timer*, void );

    // coverity[ tainted_data_return : FALSE ] version 2023.12.2
    tools::Long GetXScroll() const { return (m_aVisArea.GetWidth() * nScrollX) / 100; }
    // coverity[ tainted_data_return : FALSE ] version 2023.12.2
    tools::Long GetYScroll() const { return (m_aVisArea.GetHeight() * nScrollY) / 100; }

    SAL_DLLPRIVATE Point         AlignToPixel(const Point& rPt) const;
    SAL_DLLPRIVATE void          CalcPt( Point* pPt,const tools::Rectangle& rRect,
                                    sal_uInt16 nRangeX,
                                    sal_uInt16 nRangeY,
                                    ScrollSizeMode eScrollSizeMode);

    SAL_DLLPRIVATE bool          GetPageScrollUpOffset(SwTwips& rOff) const;
    SAL_DLLPRIVATE bool          GetPageScrollDownOffset(SwTwips& rOff) const;

    // scrollbar movements
    SAL_DLLPRIVATE bool          PageUp();
    SAL_DLLPRIVATE bool          PageDown();
    SAL_DLLPRIVATE bool          PageUpCursor(bool bSelect);
    SAL_DLLPRIVATE bool          PageDownCursor(bool bSelect);
    SAL_DLLPRIVATE void          PhyPageUp();
    SAL_DLLPRIVATE void          PhyPageDown();

    SAL_DLLPRIVATE void           CreateScrollbar( bool bHori );
    DECL_DLLPRIVATE_LINK(HoriScrollHdl, weld::Scrollbar&, void);
    DECL_DLLPRIVATE_LINK(VertScrollHdl, weld::Scrollbar&, void);
    SAL_DLLPRIVATE void EndScrollHdl(const weld::Scrollbar& rScrollbar, bool bHorizontal);
    SAL_DLLPRIVATE bool          UpdateScrollbars();
    DECL_DLLPRIVATE_LINK( WindowChildEventListener, VclWindowEvent&, void );
    SAL_DLLPRIVATE void          CalcVisArea( const Size &rPixelSz );

    // linguistics functions
    SAL_DLLPRIVATE void          HyphenateDocument();
    SAL_DLLPRIVATE bool          IsDrawTextHyphenate();
    SAL_DLLPRIVATE void          HyphenateDrawText();
    SAL_DLLPRIVATE void          StartThesaurus();

    // text conversion
    SAL_DLLPRIVATE void          StartTextConversion( LanguageType nSourceLang, LanguageType nTargetLang, const vcl::Font *pTargetFont, sal_Int32 nOptions, bool bIsInteractive );

    // used for spell checking and text conversion
    SAL_DLLPRIVATE void          SpellStart( SvxSpellArea eSpell, bool bStartDone,
                                        bool bEndDone, SwConversionArgs *pConvArgs );
    SAL_DLLPRIVATE void          SpellEnd( SwConversionArgs const *pConvArgs );

    SAL_DLLPRIVATE void          HyphStart( SvxSpellArea eSpell );
    SAL_DLLPRIVATE void          SpellContext(bool bOn = true)
                                 { m_bCenterCursor = bOn; }

    // for readonly switching
    SAL_DLLPRIVATE virtual void  Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;
    SAL_DLLPRIVATE void          CheckReadonlyState();
    SAL_DLLPRIVATE void          CheckReadonlySelection();

    // method for rotating PageDesc
    SAL_DLLPRIVATE void          SwapPageMargin(const SwPageDesc&, SvxLRSpaceItem& rLR);

    SAL_DLLPRIVATE void          SetZoom_( const Size &rEditSz,
                                      SvxZoomType eZoomType,
                                      short nFactor,
                                      bool bViewOnly);
    SAL_DLLPRIVATE void          CalcAndSetBorderPixel( SvBorder &rToFill );

    SAL_DLLPRIVATE void          ShowAtResize();

    // XForms mode: change XForms mode, based on design mode
    SAL_DLLPRIVATE void          UpdateXformsViewOption(bool bDesignMode);

    SAL_DLLPRIVATE virtual void  Move() override;

public: // #i123922# Needs to be called from a 2nd place now as a helper method
    SAL_DLLPRIVATE bool          InsertGraphicDlg( SfxRequest& );
    sal_Int32 m_nNaviExpandedStatus = -1;
    void            SetFormShell( FmFormShell* pSh )    { m_pFormShell = pSh; }
    virtual void    SelectShell();

protected:

    SwView_Impl*    GetViewImpl() {return m_pViewImpl.get();}

    void ImpSetVerb( SelectionType nSelType );

    SelectionType   GetSelectionType() const { return m_nSelectionType; }
    void            SetSelectionType(SelectionType nSet) { m_nSelectionType = nSet;}

    // for SwWebView
    void            SetShell( SfxShell* pS )            { m_pShell = pS; }

    virtual void    Activate(bool) override;
    virtual void    Deactivate(bool) override;
    virtual void    InnerResizePixel( const Point &rOfs, const Size &rSize, bool inplaceEditModeChange ) override;
    virtual void    OuterResizePixel( const Point &rOfs, const Size &rSize ) override;

    const SwFrameFormat* GetLastTableFrameFormat() const {return m_pLastTableFormat;}
    void            SetLastTableFrameFormat(const SwFrameFormat* pSet) {m_pLastTableFormat = pSet;}

    // form letter execution
    void    GenerateFormLetter(bool bUseCurrentDocument);

    using SfxShell::GetDispatcher;

public:
    SFX_DECL_VIEWFACTORY(SwView);
    SFX_DECL_INTERFACE(SW_VIEWSHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
    SfxDispatcher   &GetDispatcher();

    void                    GotFocus() const;
    virtual SdrView*        GetDrawView() const override;
    virtual bool            HasUIFeature(SfxShellFeature nFeature) const override;
    virtual void            ShowCursor( bool bOn = true ) override;
    virtual ErrCode         DoVerb(sal_Int32 nVerb) override;

    virtual sal_uInt16      SetPrinter( SfxPrinter* pNew,
                                        SfxPrinterChangeFlags nDiff = SFX_PRINTER_ALL) override;
    ShellMode               GetShellMode() const;

    css::view::XSelectionSupplier*       GetUNOObject();

    OUString                GetSelectionTextParam( bool bCompleteWords,
                                                   bool bEraseTrail );
    virtual bool            HasSelection( bool bText = true ) const override;
    virtual OUString        GetSelectionText( bool bCompleteWords = false, bool bOnlyASample = false ) override;
    virtual bool            PrepareClose( bool bUI = true ) override;
    virtual void            MarginChanged() override;

    // replace word/selection with text from the thesaurus
    // (this code has special handling for "in word" character)
    void                    InsertThesaurusSynonym( const OUString &rSynonmText, const OUString &rLookUpText, bool bValidSelection );
    bool                    IsValidSelectionForThesaurus() const;
    OUString                GetThesaurusLookUpText( bool bSelection ) const;

    // immediately switch shell -> for GetSelectionObject
    void                    StopShellTimer();

    SwWrtShell&      GetWrtShell   () const { return *m_pWrtShell; }
    SwWrtShell*      GetWrtShellPtr() const { return  m_pWrtShell.get(); }

    SwEditWin &GetEditWin()        { return *m_pEditWin; }
    const SwEditWin &GetEditWin () const { return *m_pEditWin; }

#if defined(_WIN32) || defined UNX
    void ScannerEventHdl();
#endif

    // hand the handler for text blocks to the shell; create if applicable
    SwGlossaryHdl*          GetGlosHdl();

    const tools::Rectangle& GetVisArea() const { return m_aVisArea; }

    bool            IsScroll(const tools::Rectangle& rRect) const;
    void            Scroll( const tools::Rectangle& rRect,
                            sal_uInt16 nRangeX = USHRT_MAX,
                            sal_uInt16 nRangeY = USHRT_MAX,
                            ScrollSizeMode eScrollSizeMode = ScrollSizeMode::ScrollSizeDefault);

    tools::Long            SetVScrollMax(tools::Long lMax);
    tools::Long            SetHScrollMax(tools::Long lMax);

    void            SpellError(LanguageType eLang);
    bool            ExecSpellPopup(const Point& rPt, bool bIsMouseEvent);
    void            ExecSmartTagPopup( const Point& rPt );

    DECL_DLLPRIVATE_LINK( OnlineSpellCallback, SpellCallbackInfo&, void );
    bool            ExecDrwTextSpellPopup(const Point& rPt);

    void            SetTabColFromDocPos( const Point &rPt ) { m_aTabColFromDocPos = rPt; }
    void            SetTabColFromDoc( bool b ) { m_bTabColFromDoc = b; }
    bool            IsTabColFromDoc() const    { return m_bTabColFromDoc; }
    void            SetTabRowFromDoc( bool b ) { m_bTabRowFromDoc = b; }
    bool            IsTabRowFromDoc() const    { return m_bTabRowFromDoc; }

    void            SetNumRuleNodeFromDoc( SwTextNode * pNumRuleNode )
                    { m_pNumRuleNodeFromDoc = pNumRuleNode; }

    void    DocSzChgd( const Size& rNewSize );
    const   Size&   GetDocSz() const { return m_aDocSz; }
    void    SetVisArea( const tools::Rectangle&, bool bUpdateScrollbar = true);
            void    SetVisArea( const Point&, bool bUpdateScrollbar = true);
            void    CheckVisArea();

    void RecheckBrowseMode();
    static SvxSearchDialog* GetSearchDialog();

    static sal_uInt16   GetMoveType();
    static void     SetMoveType(sal_uInt16 nSet);
    DECL_DLLPRIVATE_LINK( MoveNavigationHdl, void*, void );
    static void     SetActMark(sal_Int32 nSet);

    bool            HandleWheelCommands( const CommandEvent& );
    bool            HandleGestureZoomCommand(const CommandEvent&);
    bool            HandleGesturePanCommand(const CommandEvent&);

    // insert frames
    void            InsFrameMode(sal_uInt16 nCols);

    void            SetZoom( SvxZoomType eZoomType, short nFactor = 100, bool bViewOnly = false);
    virtual void    SetZoomFactor( const Fraction &rX, const Fraction & ) override;

    void            SetViewLayout( sal_uInt16 nColumns, bool bBookMode, bool bViewOnly = false );

    void            ShowHScrollbar(bool bShow);
    bool            IsHScrollbarVisible()const;

    void            ShowVScrollbar(bool bShow);
    bool            IsVScrollbarVisible()const;

    void            EnableHScrollbar(bool bEnable);
    void            EnableVScrollbar(bool bEnable);

    void            CreateVRuler();
    void            KillVRuler();
    void            CreateTab();
    void            KillTab();

    bool            StatVRuler() const { return m_pVRuler->IsVisible(); }
    void            ChangeVRulerMetric(FieldUnit eUnit);
    void            GetVRulerMetric(FieldUnit& rToFill) const;

    SvxRuler&       GetHRuler()    { return *m_pHRuler; }
    SvxRuler&       GetVRuler()    { return *m_pVRuler; }
    void            InvalidateRulerPos();
    void            ChangeTabMetric(FieldUnit eUnit);
    void            GetHRulerMetric(FieldUnit& rToFill) const;

    // Handler
    void            Execute(SfxRequest&);
    void            ExecLingu(SfxRequest&);
    void            ExecDlg(SfxRequest const &);
    void            ExecDlgExt(SfxRequest&);
    void            ExecColl(SfxRequest const &);
    void            ExecutePrint(SfxRequest&);
    void            ExecDraw(const SfxRequest&);
    void            ExecTabWin(SfxRequest const &);
    void            ExecuteStatusLine(SfxRequest&);
    DECL_DLLPRIVATE_LINK( ExecRulerClick, Ruler *, void );
    void            ExecSearch(SfxRequest&);
    void            ExecViewOptions(SfxRequest &);

    virtual bool    IsConditionalFastCall( const SfxRequest &rReq ) override;

    void            StateViewOptions(SfxItemSet &);
    void            StateSearch(SfxItemSet &);
    void            GetState(SfxItemSet&);
    void            StateStatusLine(SfxItemSet&);
    void            UpdateWordCount(SfxShell*, sal_uInt16);
    void            ExecFormatFootnote();
    void            ExecNumberingOutline(SfxItemPool &);

    // functions for drawing
    void            SetDrawFuncPtr(std::unique_ptr<SwDrawBase> pFuncPtr);
    SwDrawBase*     GetDrawFuncPtr() const  { return m_pDrawActual.get(); }
    void            GetDrawState(SfxItemSet &rSet);
    void            ExitDraw();
    bool     IsDrawRotate() const      { return m_bDrawRotate; }
    void     FlipDrawRotate()    { m_bDrawRotate = !m_bDrawRotate; }
    bool     IsDrawSelMode() const     { return m_bDrawSelMode; }
    void            SetSelDrawSlot();
    void     FlipDrawSelMode()   { m_bDrawSelMode = !m_bDrawSelMode; }
    void            NoRotate();     // turn off rotate mode
    void            ToggleRotate();     // switch between move and rotate mode

    bool            EnterDrawTextMode(const Point& aDocPos);
    /// Same as EnterDrawTextMode(), but takes an SdrObject instead of guessing it by document position.
    bool EnterShapeDrawTextMode(SdrObject* pObject);
    void            LeaveDrawCreate()   { m_nDrawSfxId = m_nFormSfxId = USHRT_MAX; m_sDrawCustom.clear(); m_eFormObjKind = SdrObjKind::NONE; }
    bool            IsDrawMode() const  { return (m_nDrawSfxId != USHRT_MAX || m_nFormSfxId != USHRT_MAX); }
    bool            IsFormMode() const;
    bool            IsBezierEditMode() const;
    bool            AreOnlyFormsSelected() const;
    bool            HasOnlyObj(SdrObject const *pSdrObj, SdrInventor eObjInventor) const;
    bool            BeginTextEdit(  SdrObject* pObj, SdrPageView* pPV=nullptr,
                                    vcl::Window* pWin=nullptr, bool bIsNewObj=false, bool bSetSelectionToStart=false );
    bool isSignatureLineSelected() const;
    bool isSignatureLineSigned() const;
    bool isQRCodeSelected() const;

    void            StateTabWin(SfxItemSet&);

    // attributes have changed
    DECL_LINK( AttrChangedNotify, LinkParamNone*, void );

    // form control has been activated
    DECL_DLLPRIVATE_LINK( FormControlActivated, LinkParamNone*, void );

    // edit links
    void            EditLinkDlg();
    void            AutoCaption(const sal_uInt16 nType, const SvGlobalName *pOleId = nullptr);
    void            InsertCaption(const InsCaptionOpt *pOpt);

    // Async call by Core
    void UpdatePageNums();

    OUString    GetPageStr(sal_uInt16 nPhyNum, sal_uInt16 nVirtNum, const OUString& rPgStr);

    /// Force page size for PgUp/PgDown to overwrite the computation based on m_aVisArea.
    void ForcePageUpDownOffset(SwTwips nTwips)
    {
        m_nLOKPageUpDownOffset = nTwips;
    }

    // hand over Shell
    SfxShell       *GetCurShell()  { return m_pShell; }
    SwDocShell     *GetDocShell();
    const SwDocShell *GetDocShell() const { return const_cast<SwView*>(this)->GetDocShell(); }

    virtual       FmFormShell    *GetFormShell()       override { return m_pFormShell; }
    virtual const FmFormShell    *GetFormShell() const override { return m_pFormShell; }

    // so that in the SubShells' DTors m_pShell can be reset if applicable
    void ResetSubShell()    { m_pShell = nullptr; }

    virtual void    WriteUserData(OUString &, bool bBrowse = false) override;
    virtual void    ReadUserData(const OUString &, bool bBrowse = false) override;
    virtual void    ReadUserDataSequence ( const css::uno::Sequence < css::beans::PropertyValue >& ) override;
    virtual void    WriteUserDataSequence ( css::uno::Sequence < css::beans::PropertyValue >& ) override;

    void SetCursorAtTop( bool bFlag, bool bCenter = false )
        { m_bTopCursor = bFlag; m_bCenterCursor = bCenter; }

    bool JumpToSwMark( const SwMarkName& rMark );

    tools::Long InsertDoc( sal_uInt16 nSlotId, const OUString& rFileName,
                    const OUString& rFilterName, sal_Int16 nVersion = 0 );

    void ExecuteInsertDoc( SfxRequest& rRequest, const SfxPoolItem* pItem );
    tools::Long InsertMedium( sal_uInt16 nSlotId, std::unique_ptr<SfxMedium> pMedium, sal_Int16 nVersion );
    DECL_DLLPRIVATE_LINK( DialogClosedHdl, sfx2::FileDialogHelper *, void );

    // status methods for clipboard.
    // Status changes now notified from the clipboard.
    bool IsPasteAllowed();
    bool IsPasteSpecialAllowed();
    bool IsPasteSpreadsheet(bool bHasOwnTableCopied);

    // Enable mail merge - mail merge field dialog enabled
    void EnableMailMerge();

    SwView(SfxViewFrame& rFrame, SfxViewShell*);
    virtual ~SwView() override;

    void SetDying() override;

    void NotifyDBChanged();

    SfxObjectShellLock CreateTmpSelectionDoc();

    void        AddTransferable(SwTransferable& rTransferable);

    // store MailMerge data while "Back to Mail Merge Wizard" FloatingWindow is active
    // or to support printing
    void SetMailMergeConfigItem(std::shared_ptr<SwMailMergeConfigItem> const & rConfigItem);
    std::shared_ptr<SwMailMergeConfigItem> const & GetMailMergeConfigItem() const;
    std::shared_ptr<SwMailMergeConfigItem> EnsureMailMergeConfigItem(const SfxItemSet* pArgs = nullptr);

    OUString GetDataSourceName() const;
    static bool IsDataSourceAvailable(const OUString& sDataSourceName);

    void ExecFormatPaintbrush(SfxRequest const &);
    void StateFormatPaintbrush(SfxItemSet &);

    // public for D&D
    ErrCode     InsertGraphic( const OUString &rPath, const OUString &rFilter,
                            bool bLink, GraphicFilter *pFlt );

    void ExecuteScan( SfxRequest& rReq );

    SwPostItMgr* GetPostItMgr() { return m_pPostItMgr.get();}
    const SwPostItMgr* GetPostItMgr() const { return m_pPostItMgr.get();}
    
    sfx2::DocumentTimer* GetDocumentTimer() { return m_pDocumentTimer.get(); }
    const sfx2::DocumentTimer* GetDocumentTimer() const { return m_pDocumentTimer.get(); }

    // exhibition hack (MA,MBA)
    void SelectShellForDrop();

    void UpdateDocStats();

    void SetMaxOutlineLevelShown(int nLevel) {m_nMaxOutlineLevelShown = nLevel;}
    int GetMaxOutlineLevelShown() const {return m_nMaxOutlineLevelShown;}

    // methods for printing
    SAL_DLLPRIVATE virtual   SfxPrinter*     GetPrinter( bool bCreate = false ) override;
    SAL_DLLPRIVATE virtual bool  HasPrintOptionsPage() const override;
    SAL_DLLPRIVATE virtual std::unique_ptr<SfxTabPage> CreatePrintOptionsPage(weld::Container* pPage, weld::DialogController* pController,
                                                    const SfxItemSet& rSet) override;
    static SvxSearchItem* GetSearchItem() { return s_pSrchItem; }
    static void SetSearchItem(SvxSearchItem* pSearchItem) { s_pSrchItem = pSearchItem; }

    /// See SfxViewShell::getPart().
    int getPart() const override;
    /// See SfxViewShell::dumpAsXml().
    void dumpAsXml(xmlTextWriterPtr pWriter) const override;
    void SetRedlineAuthor(const OUString& rAuthor);
    const OUString& GetRedlineAuthor() const;
    /// See SfxViewShell::afterCallbackRegistered().
    void afterCallbackRegistered() override;
    /// See SfxViewShell::NotifyCursor().
    void NotifyCursor(SfxViewShell* pViewShell) const override;
    /// See SfxViewShell::GetColorConfigColor().
    ::Color GetColorConfigColor(svtools::ColorConfigEntry nColorType) const override;

    void SetUIElementVisibility(const OUString& sElementURL, bool bShow) const;
    void ShowUIElement(const OUString& sElementURL) const;
    void HideUIElement(const OUString& sElementURL) const;

    enum CachedStringID
    {
        OldGrfCat,
        OldTabCat,
        OldFrameCat,
        OldDrwCat,
        CachedStrings
    };

    OUString m_StringCache[CachedStrings];

    const OUString& GetCachedString(CachedStringID id)
    {
        return m_StringCache[id];
    }

    void SetCachedString(CachedStringID id, const OUString& sStr)
    {
        m_StringCache[id] = sStr;
    }

    const OUString& GetOldGrfCat();
    void SetOldGrfCat(const OUString& sStr);
    const OUString& GetOldTabCat();
    void SetOldTabCat(const OUString& sStr);
    const OUString& GetOldFrameCat();
    void SetOldFrameCat(const OUString& sStr);
    const OUString& GetOldDrwCat();
    void SetOldDrwCat(const OUString& sStr);

    virtual tools::Rectangle getLOKVisibleArea() const override;
    virtual void flushPendingLOKInvalidateTiles() override;
    virtual std::optional<OString> getLOKPayload(int nType, int nViewId) const override;

    bool IsHighlightCharDF() const { return m_bIsHighlightCharDF; }
    bool IsSpotlightParaStyles() const { return m_bIsSpotlightParaStyles; }
    bool IsSpotlightCharStyles() const { return m_bIsSpotlightCharStyles; }

private:
    AutoTimer m_aBringToAttentionBlinkTimer;
    size_t m_nBringToAttentionBlinkTimeOutsRemaining;

    std::unique_ptr<sdr::overlay::OverlayObject> m_xBringToAttentionOverlayObject;

    DECL_LINK(BringToAttentionBlinkTimerHdl, Timer*, void);

public:
    void BringToAttention(std::vector<basegfx::B2DRange>&& aRanges = {});
    void BringToAttention(const tools::Rectangle& rRect);
    void BringToAttention(const SwNode* pNode);
};

std::unique_ptr<SfxTabPage> CreatePrintOptionsPage(weld::Container* pPage, weld::DialogController* pController,
                                          const SfxItemSet &rOptions,
                                          bool bPreview);

extern bool bDocSzUpdated;

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
