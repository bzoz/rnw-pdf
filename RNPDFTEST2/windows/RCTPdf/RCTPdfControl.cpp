#include "pch.h"
#include "RCTPdfControl.h"
#if __has_include("RCTPdfControl.g.cpp")
#include "RCTPdfControl.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Microsoft::ReactNative;
using namespace Windows::Data::Json;
using namespace Windows::Data::Pdf;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Storage::Pickers;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;

// C:\\Users\\ja\Desktop\\bod.pdf

namespace winrt::RCTPdf::implementation
{
  PDFPageInfo::PDFPageInfo(winrt::Windows::UI::Xaml::Controls::Image image, winrt::Windows::Data::Pdf::PdfPage page, double imageScale, double renderScale) :
    image(image), page(page), imageScale(imageScale), renderScale(renderScale), scaledTopOffset(0), scaledLeftOffset(0) {
    auto dims = page.Size();
    height = (unsigned)dims.Height;
    width = (unsigned)dims.Width;
    scaledHeight = (unsigned)(height * imageScale);
    scaledWidth = (unsigned)(width * imageScale);
  }
  PDFPageInfo::PDFPageInfo(const PDFPageInfo& rhs) :
    height(rhs.height), width(rhs.width), scaledHeight(rhs.scaledHeight), scaledWidth(rhs.scaledWidth),
    scaledTopOffset(rhs.scaledTopOffset), scaledLeftOffset(rhs.scaledTopOffset), imageScale(rhs.imageScale),
    renderScale((double)rhs.renderScale), image(rhs.image), page(rhs.page)
  { }
  PDFPageInfo::PDFPageInfo(PDFPageInfo&& rhs) :
    height(rhs.height), width(rhs.width), scaledHeight(rhs.scaledHeight), scaledWidth(rhs.scaledWidth),
    scaledTopOffset(rhs.scaledTopOffset), scaledLeftOffset(rhs.scaledTopOffset), imageScale(rhs.imageScale),
    renderScale((double)rhs.renderScale), image(std::move(rhs.image)), page(std::move(rhs.page))
  { }
  unsigned PDFPageInfo::pageVisiblePixels(bool horizontal, double viewportStart, double viewportEnd) const {
    if (viewportEnd < viewportStart)
      std::swap(viewportStart, viewportEnd);
    auto pageStart = horizontal ? scaledLeftOffset : scaledTopOffset;
    auto pageSize = PDFPageInfo::pageSize(horizontal);
    auto pageEnd = pageStart + pageSize;
    auto uViewportStart = (unsigned)viewportStart;
    auto uViewportEnd = (unsigned)viewportEnd;
    if (pageStart >= uViewportStart && pageStart <= uViewportEnd) { // we see the top edge
      return (std::min)(pageEnd, uViewportEnd) - pageStart;
    }
    if (pageEnd >= uViewportStart && pageEnd <= uViewportEnd) { // we see the bottom edge
      return pageEnd - (std::max)(pageStart, uViewportStart);
    }
    if (pageStart <= uViewportStart && pageEnd >= uViewportEnd) {// we see the entire page
      return uViewportEnd - uViewportStart;
    }
    return 0;
  }
  unsigned PDFPageInfo::pageSize(bool horizontal) const {
    return horizontal ? scaledWidth : scaledHeight;
  }
  bool PDFPageInfo::needsRender() const {
    double currentRenderScale = renderScale;
    return currentRenderScale < imageScale || currentRenderScale > imageScale * 2;
  }
  winrt::IAsyncAction PDFPageInfo::render() {
    return render(imageScale);
  }
  winrt::IAsyncAction PDFPageInfo::render(double useScale) {
    double currentRenderScale;
    while (true) {
      currentRenderScale = renderScale;
      if (!(currentRenderScale < imageScale || currentRenderScale > imageScale * 2))
        co_return;
      if (renderScale.compare_exchange_weak(currentRenderScale, useScale))
        break;
      if (renderScale > useScale)
        co_return;
    }
    PdfPageRenderOptions renderOptions;
    auto dims = page.Size();
    renderOptions.DestinationHeight(static_cast<uint32_t>(dims.Height * useScale));
    renderOptions.DestinationWidth(static_cast<uint32_t>(dims.Width * useScale));
    InMemoryRandomAccessStream stream;
    co_await page.RenderToStreamAsync(stream, renderOptions);
    BitmapImage bitmap;
    co_await bitmap.SetSourceAsync(stream);
    if (renderScale == useScale)
      image.Source(bitmap);
  }
  
  RCTPdfControl::RCTPdfControl(IReactContext const& reactContext) : m_reactContext(reactContext) {
    this->AllowFocusOnInteraction(true);
    InitializeComponent();
    m_viewChangedRevoker = PagesContainer().ViewChanged(winrt::auto_revoke,
      [ref = get_weak()](auto const& sender, auto const& args) {
      if (auto self = ref.get()) {
        self->OnViewChanged(sender, args);
      }
    });
  }

  winrt::Windows::Foundation::Collections::
    IMapView<winrt::hstring, winrt::Microsoft::ReactNative::ViewManagerPropertyType>
    RCTPdfControl::NativeProps() noexcept {
    auto nativeProps = winrt::single_threaded_map<hstring, ViewManagerPropertyType>();
    nativeProps.Insert(L"path", ViewManagerPropertyType::String);
    nativeProps.Insert(L"page", ViewManagerPropertyType::Number);
    // TODO: nativeProps.Insert(L"scale", ViewManagerPropertyType::Number);
    // TODO: nativeProps.Insert(L"minScale", ViewManagerPropertyType::Number);
    // TODO: nativeProps.Insert(L"maxScale", ViewManagerPropertyType::Number);
    // TODO: nativeProps.Insert(L"horizontal", ViewManagerPropertyType::Boolean);
    // TODO: nativeProps.Insert(L"fitWidth", ViewManagerPropertyType::Boolean);
    // TODO: nativeProps.Insert(L"fitPolicy", ViewManagerPropertyType::Number);
    // TODO: nativeProps.Insert(L"spacing", ViewManagerPropertyType::Number);
    nativeProps.Insert(L"password", ViewManagerPropertyType::String);
    // TODO: nativeProps.Insert(L"background", ViewManagerPropertyType::Color);
    // TODO: nativeProps.Insert(L"enablePaging", ViewManagerPropertyType::Boolean);
    // TODO: nativeProps.Insert(L"enableRTL", ViewManagerPropertyType::Boolean);
    // TODO: nativeProps.Insert(L"singlePage", ViewManagerPropertyType::Boolean);

    return nativeProps.GetView();
  }

  void RCTPdfControl::UpdateProperties(winrt::Microsoft::ReactNative::IJSValueReader const& propertyMapReader) noexcept {
    std::unique_lock lock(m_rwlock);
    const JSValueObject& propertyMap = JSValue::ReadObjectFrom(propertyMapReader);
    std::string pdfURI;
    std::string pdfPassword;
    m_currentPage = 0;
    for (auto const& pair : propertyMap) {
      auto const& propertyName = pair.first;
      auto const& propertyValue = pair.second;
      if (propertyName == "path") {
        pdfURI = propertyValue != nullptr ? propertyValue.AsString() : "";
      }
      else if (propertyName == "password") {
        pdfPassword = propertyValue != nullptr ? propertyValue.AsString() : "";
      }
      else if (propertyName == "page") {
        m_currentPage = propertyValue != nullptr ? propertyValue.AsInt32() - 1 : 0;
      }

    }
    if (pdfURI != m_pdfURI || pdfPassword != m_pdfPassword) {
      m_pdfURI = pdfURI;
      m_pdfPassword = pdfPassword;
      LoadPDF(std::move(lock), 2);
    }
  }

  winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomBubblingEventTypeConstants() noexcept {
    return nullptr;
  }
  winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomDirectEventTypeConstants() noexcept {
    return [](IJSValueWriter const& constantWriter) {
      WriteCustomDirectEventTypeConstant(constantWriter, "Error");
      WriteCustomDirectEventTypeConstant(constantWriter, "LoadComplete");
      WriteCustomDirectEventTypeConstant(constantWriter, "PageChanged");
      WriteCustomDirectEventTypeConstant(constantWriter, "ScaleChanged");
    };
  }
  void RCTPdfControl::SignalError(const std::string& error) {
    m_reactContext.DispatchEvent(
      *this,
      L"topError",
      [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
        eventDataWriter.WriteObjectBegin();
        {
          WriteProperty(eventDataWriter, L"error", winrt::to_hstring(error));
        }
        eventDataWriter.WriteObjectEnd();
      });
  }
  void RCTPdfControl::SignalLoadComplete(int totalPages, int width, int height) {
    m_reactContext.DispatchEvent(
      *this,
      L"topLoadComplete",
      [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
        eventDataWriter.WriteObjectBegin();
        {
          WriteProperty(eventDataWriter, L"totalPages", totalPages);
          WriteProperty(eventDataWriter, L"width", width);
          WriteProperty(eventDataWriter, L"height", height);
        }
        eventDataWriter.WriteObjectEnd();
      });
  }
  void RCTPdfControl::SignalPageChange(int page, int totalPages) {
    m_reactContext.DispatchEvent(
      *this,
      L"topPageChanged",
      [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
        eventDataWriter.WriteObjectBegin();
        {
          WriteProperty(eventDataWriter, L"page", page);
          WriteProperty(eventDataWriter, L"totalPages", totalPages);
        }
        eventDataWriter.WriteObjectEnd();
      });
  }
  void RCTPdfControl::SignalScaleChanged(double scale) {
    m_reactContext.DispatchEvent(
      *this,
      L"topScaleChanged",
      [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
        eventDataWriter.WriteObjectBegin();
        {
          WriteProperty(eventDataWriter, L"scale", scale);
        }
        eventDataWriter.WriteObjectEnd();
      });
  }

  winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> RCTPdfControl::Commands() noexcept {
    auto commands = winrt::single_threaded_vector<hstring>();
    commands.Append(L"setPage");
    return commands.GetView();
  }

  void RCTPdfControl::DispatchCommand(winrt::hstring const& commandId, winrt::Microsoft::ReactNative::IJSValueReader const& commandArgsReader) noexcept {
    auto commandArgs = JSValue::ReadArrayFrom(commandArgsReader);
    if (commandId == L"setPage" && commandArgs.size() > 0) {
      std::shared_lock lock(m_rwlock);
      auto page = commandArgs[0].AsInt32() - 1;
      GoToPage(page);
    }
  }

  void RCTPdfControl::UpdatePagesInfoMarginOrScale() {
    unsigned scaledMargin = (unsigned)(m_scale * m_margins);
    for (auto& page : m_pages) {
      page.imageScale = m_scale;
      page.scaledWidth = (unsigned)(page.width * m_scale);
      page.scaledHeight = (unsigned)(page.height * m_scale);
      page.image.Margin(ThicknessHelper::FromUniformLength(scaledMargin));
      page.image.Width(page.scaledWidth);
      page.image.Height(page.scaledHeight);
    }
    unsigned totalTopOffset = 0;
    unsigned totalLeftOffset = 0;
    unsigned doubleScaledMargin = scaledMargin * 2;
    if (m_reverse) {
      for (int page = m_pages.size() - 1; page >= 0; --page) {
        m_pages[page].scaledTopOffset = totalTopOffset;
        totalTopOffset += m_pages[page].scaledHeight + doubleScaledMargin;
        m_pages[page].scaledLeftOffset = totalLeftOffset;
        totalLeftOffset += m_pages[page].scaledWidth + doubleScaledMargin;
      }
    } else {
      for (int page = 0; page < (int)m_pages.size(); ++page) {
        m_pages[page].scaledTopOffset = totalTopOffset;
        totalTopOffset += m_pages[page].scaledHeight + doubleScaledMargin;
        m_pages[page].scaledLeftOffset = totalLeftOffset;
        totalLeftOffset += m_pages[page].scaledWidth + doubleScaledMargin;
      }
    }
  }

  winrt::fire_and_forget RCTPdfControl::LoadPDF(std::unique_lock<std::shared_mutex> lock, int fitPolicy) {
    auto lifetime = get_strong();
    auto uri = Uri(winrt::to_hstring(m_pdfURI));
    auto file = co_await StorageFile::GetFileFromApplicationUriAsync(uri);
    auto document = co_await PdfDocument::LoadFromFileAsync(file, winrt::to_hstring(m_pdfPassword));
    auto items = Pages().Items();
    items.Clear();
    m_pages.clear();
    auto panelTemplate = FindName(winrt::to_hstring("OrientationSelector")).try_as<StackPanel>();
    if (panelTemplate) {
      panelTemplate.Orientation(m_horizontal ? Orientation::Horizontal : Orientation::Vertical);
    }
    if (document.PageCount() == 0) {
      if (fitPolicy != -1)
        m_scale = 1;
    } else {
      auto firstPageSize = document.GetPage(0).Size();
      auto viewWidth = PagesContainer().ViewportWidth();
      auto viewHeight = PagesContainer().ViewportHeight();
      switch (fitPolicy) {
      case 0:
        m_scale = viewWidth / (firstPageSize.Width + 2 * m_margins);
        break;
      case 1:
        m_scale = viewHeight / (firstPageSize.Height + 2 * m_margins);
        break;
      case 2:
        m_scale = (std::min)(viewWidth / (firstPageSize.Width + 2 * m_margins), viewHeight / (firstPageSize.Height + 2 * m_margins));
        break;
      default:
        break;
      }
    }
    for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
      auto page = document.GetPage(pageIdx);
      auto dims = page.Size();
      Image pageImage;
      pageImage.HorizontalAlignment(HorizontalAlignment::Center);
      m_pages.emplace_back(pageImage, page, m_scale, 0);
    }
    if (m_reverse) {
      for (int page = m_pages.size() - 1; page >= 0; --page)
        items.Append(m_pages[page].image);
    } else {
      for (int page = 0; page < (int) m_pages.size(); ++page)
        items.Append(m_pages[page].image);
    }
    UpdatePagesInfoMarginOrScale();
    if (m_currentPage < 0)
      m_currentPage = 0;
    if (m_currentPage < (int)m_pages.size()) {
      co_await m_pages[m_currentPage].render();
      GoToPage(m_currentPage);
    }
    if (m_pages.empty()) {
      SignalLoadComplete(0, 0, 0);
    } else {
      SignalLoadComplete(m_pages.size(), m_pages.front().width, m_pages.front().height);
    }
    lock.unlock();
    // Render low-res preview of the pages
    std::shared_lock shared_lock(m_rwlock);
    double useScale = (std::min)(m_scale, 0.1);
    for (unsigned page = 0; page < m_pages.size(); ++page) {
      co_await m_pages[page].render(useScale);
    }
  }

  winrt::fire_and_forget RCTPdfControl::OnViewChanged(winrt::Windows::Foundation::IInspectable const&,
    winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const& args) {
    auto lifetime = get_strong();
    auto container = PagesContainer();
    auto currentHorizontalOffset = container.HorizontalOffset();
    auto currentVerticalOffset = container.VerticalOffset();
    double offsetStart = m_horizontal ? currentHorizontalOffset : currentVerticalOffset;
    double viewSize = m_horizontal ? container.ViewportWidth() : container.ViewportHeight();
    double offsetEnd = offsetStart + viewSize;
    std::shared_lock lock(m_rwlock, std::defer_lock);
    if (args.IsIntermediate() || !lock.try_lock() || viewSize == 0 || m_pages.empty())
      return;
    // Go through pages untill we reach a visible page
    int page = 0;
    double visiblePagePixels = 0;
    for (; page < (int)m_pages.size(); ++page) {
      visiblePagePixels = m_pages[page].pageVisiblePixels(m_horizontal, offsetStart, offsetEnd);
      if (visiblePagePixels > 0)
        break;
    }
    if (page == (int)m_pages.size()) {
      --page;
    } else {
      double pagePixels = m_pages[page].pageSize(m_horizontal);
      // #"page" is the first visible page. Check how much of the view port this page covers...
      double viewCoveredByPage = visiblePagePixels / viewSize;
      // ...and how much of the page is visible:
      double pageVisiblePart = visiblePagePixels / pagePixels;
      // If:
      //  - less than 50% of the screen is covered by the page
      //  - less than 50% of the page is visible (important if more than one page fits the screen)
      //  - there is a next page
      // move the indicator to that page:
      if (viewCoveredByPage < 0.5 && pageVisiblePart < 0.5 && page + 1 < (int)m_pages.size()) {
        ++page;
      }
    }
    // Render all visible pages - first the curent one, then the next visible ones and one
    // more, then the one before that might be partly visible, then one more before
    if (m_pages[page].needsRender()) {
      SignalError("Rendering " + std::to_string(page + 1));
      co_await m_pages[page].render();
    }
    auto pageToRender = page + 1;
    while (pageToRender < (int)m_pages.size() &&
           m_pages[pageToRender].pageVisiblePixels(m_horizontal, offsetStart, offsetEnd) > 0) {
      if (m_pages[pageToRender].needsRender()) {
        SignalError("Rendering " + std::to_string(pageToRender + 1));
        co_await m_pages[pageToRender].render();
      }
      ++pageToRender;
    }
    if (pageToRender < (int)m_pages.size() && m_pages[pageToRender].needsRender()) {
      SignalError("Rendering " + std::to_string(pageToRender + 1));
      co_await m_pages[pageToRender].render();
    }
    if (page >= 1 && m_pages[page - 1].needsRender()) {
      SignalError("Rendering " + std::to_string(page));
      co_await m_pages[page - 1].render();
    }
    if (page >= 2 && m_pages[page - 2].needsRender()) {
      SignalError("Rendering " + std::to_string(page -1));
      co_await m_pages[page - 2].render();
    }
    if (page != m_currentPage) {
      m_currentPage = page;
      SignalPageChange(m_currentPage + 1, m_pages.size());
    }
  }

  void RCTPdfControl::GoToPage(int page) {
    if (page < 0 || page > (int)m_pages.size()) {
      return;
    }
    auto neededOffset = m_horizontal ? m_pages[page].scaledLeftOffset : m_pages[page].scaledTopOffset;
    double horizontalOffset = m_horizontal ? neededOffset : PagesContainer().HorizontalOffset();
    double verticalOffset = m_horizontal ? PagesContainer().VerticalOffset() : neededOffset;
    PagesContainer().ChangeView(horizontalOffset, verticalOffset, nullptr, true);
  }

  void RCTPdfControl::PagesContainer_PointerWheelChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& e)
  {
    auto self = get_strong();
    winrt::Windows::System::VirtualKeyModifiers modifiers = e.KeyModifiers();
    if ((modifiers & winrt::Windows::System::VirtualKeyModifiers::Control) != winrt::Windows::System::VirtualKeyModifiers::Control)
      return;
    double delta = (e.GetCurrentPoint(*this).Properties().MouseWheelDelta() / WHEEL_DELTA) * 0.1;
    std::shared_lock lock(m_rwlock);
    auto newScale = (std::max)((std::min)(m_scale + delta, m_maxScale), m_minScale);
    if (newScale != m_scale) {
      double rescale = newScale / m_scale;
      double targetHorizontalOffset = PagesContainer().HorizontalOffset() * rescale;
      double targetVerticalOffset = PagesContainer().VerticalOffset() * rescale;
      m_scale = newScale;
      UpdatePagesInfoMarginOrScale();
      auto maxHorizontalOffset = PagesContainer().ScrollableWidth();
      auto maxVerticalOffset = PagesContainer().ScrollableHeight();
      PagesContainer().ChangeView(min(targetHorizontalOffset, maxHorizontalOffset), min(targetVerticalOffset, maxVerticalOffset), nullptr, true);
      SignalScaleChanged(m_scale);
    }
    e.Handled(true);
  }
}

