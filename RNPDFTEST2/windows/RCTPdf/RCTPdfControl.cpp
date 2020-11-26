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
    height = dims.Height;
    width = dims.Width;
    scaledHeight = height * imageScale;
    scaledWidth = width * imageScale;
  }

  double PDFPageInfo::pageVisiblePixels(bool horizontal, double viewportStart, double viewportEnd) const {
    if (viewportEnd < viewportStart)
      std::swap(viewportStart, viewportEnd);
    auto pageStart = horizontal ? scaledLeftOffset : scaledTopOffset;
    auto pageSize = PDFPageInfo::pageSize(horizontal);
    auto pageEnd = pageStart + pageSize;
    if (pageStart >= viewportStart && pageStart <= viewportEnd) { // we see the top edge
      return (std::min)(pageEnd, viewportEnd) - pageStart;
    }
    if (pageEnd >= viewportStart && pageEnd <= viewportEnd) { // we see the bottom edge
      return pageEnd - (std::max)(pageStart, viewportStart);
    }
    if (pageStart <= viewportStart && pageEnd >= viewportEnd) {// we see the entire page
      return viewportEnd - viewportStart;
    }
    return 0;
  }
  double PDFPageInfo::pageSize(bool horizontal) const {
    return horizontal ? scaledWidth : scaledHeight;
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
    std::unique_lock lock(m_mutex);
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
      LoadPDF(std::move(lock));
    }
  }

  winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomBubblingEventTypeConstants() noexcept {
    return nullptr;
  }

  winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomDirectEventTypeConstants() noexcept {
    return [](IJSValueWriter const& constantWriter) {
      // TODO: WriteCustomDirectEventTypeConstant(constantWriter, "onLoadComplete");
      WriteCustomDirectEventTypeConstant(constantWriter, "PageChanged");
      WriteCustomDirectEventTypeConstant(constantWriter, "onError");
    };
  }

  winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> RCTPdfControl::Commands() noexcept {
    auto commands = winrt::single_threaded_vector<hstring>();
    // TODO: commands.Append(L"setPage");
    // TODO: commands.Append(L"setScale");
    return commands.GetView();
  }

  void RCTPdfControl::DispatchCommand(winrt::hstring const& commandId, winrt::Microsoft::ReactNative::IJSValueReader const& commandArgsReader) noexcept {
    // TODO: handle commands here
    auto commandArgs = JSValue::ReadArrayFrom(commandArgsReader);
    if (commandId == L"sampleCommand") {
      //TextElement().Text(L"sampleCommand used!");
    }
  }

  static winrt::IAsyncOperation<BitmapImage> renderPDFPage(PdfPage page, double scale) {
    PdfPageRenderOptions renderOptions;
    auto dims = page.Size();
    renderOptions.DestinationHeight(static_cast<uint32_t>(dims.Height * scale));
    renderOptions.DestinationWidth(static_cast<uint32_t>(dims.Width * scale));
    InMemoryRandomAccessStream stream;
    co_await page.RenderToStreamAsync(stream, renderOptions);
    BitmapImage image;
    co_await image.SetSourceAsync(stream);
    return image;
  }

  void RCTPdfControl::UpdatePagesInfoMarginOrScale() {
    for (auto& page : m_pages) {
      page.imageScale = m_scale;
      page.scaledWidth = page.width * m_scale;
      page.scaledHeight = page.height * m_scale;
      page.image.Margin(ThicknessHelper::FromUniformLength(m_margins * m_scale));
      page.image.Width(page.scaledWidth);
      page.image.Height(page.scaledHeight);
    }
    double totalTopOffset = 0;
    double totalLeftOffset = 0;
    if (m_reverse) {
      for (int page = m_pages.size() - 1; page >= 0; --page) {
        m_pages[page].scaledTopOffset = totalTopOffset;
        totalTopOffset += m_pages[page].scaledHeight + m_margins * 2 * m_scale;
        m_pages[page].scaledLeftOffset = totalLeftOffset;
        totalLeftOffset += m_pages[page].scaledWidth + m_margins * 2 * m_scale;
      }
    } else {
      for (int page = 0; page < (int)m_pages.size(); ++page) {
        m_pages[page].scaledTopOffset = totalTopOffset;
        totalTopOffset += m_pages[page].scaledHeight + m_margins * 2 * m_scale;
        m_pages[page].scaledLeftOffset = totalLeftOffset;
        totalLeftOffset += m_pages[page].scaledWidth + m_margins * 2 * m_scale;
      }
    }
  }

  winrt::IAsyncAction RCTPdfControl::UpadtePageRender(int page) {
    auto& pageInfo = m_pages[page];
    pageInfo.renderScale = m_scale;
    if (m_pages[page].renderScale > m_scale && m_pages[page].renderScale < m_scale * 0.5)
      co_return;
    pageInfo.image.Source(co_await renderPDFPage(pageInfo.page, m_scale));
  }

  winrt::fire_and_forget RCTPdfControl::UpdateMultiplePagesRender(int firstPage, int endPage) {
    std::unique_lock lock(m_mutex);
    auto currentLoadIndex = m_pdfLoadedIndex;
    for (unsigned page = firstPage; page < endPage && page < m_pages.size(); ++page) {
      // Upscale always, downscale if over 50%
      if (m_pages[page].renderScale > m_scale && m_pages[page].renderScale < m_scale * 0.5)
        continue;
      auto pdfPage = m_pages[page].page;
      auto scale = m_scale;
      lock.unlock();
      auto bitmap = co_await renderPDFPage(pdfPage, scale);
      lock.lock();
      if (currentLoadIndex != m_pdfLoadedIndex || scale != m_scale)
        co_return;
      m_pages[page].renderScale = scale;
      m_pages[page].image.Source(bitmap);
    }
  }

  winrt::fire_and_forget RCTPdfControl::LoadPDF(std::unique_lock<std::mutex> lock) {
    auto lifetime = get_strong();
    ++m_pdfLoadedIndex;
    auto uri = Uri(winrt::to_hstring(m_pdfURI));
    auto file = co_await StorageFile::GetFileFromApplicationUriAsync(uri);
    auto document = co_await PdfDocument::LoadFromFileAsync(file, winrt::to_hstring(m_pdfPassword));
    auto items = Pages().Items();
    items.Clear();
    m_pages.clear();
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
    co_await UpadtePageRender(m_currentPage);
    GoToPage(m_currentPage);
    auto pagesCount = m_pages.size();
    lock.unlock();
    UpdateMultiplePagesRender(0, pagesCount);
  }

  void RCTPdfControl::OnViewChanged(winrt::Windows::Foundation::IInspectable const&,
    winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const&) {
    std::unique_lock lock(m_mutex);
    auto container = PagesContainer();
    double offsetStart = m_horizontal ? container.HorizontalOffset() : container.VerticalOffset();
    double viewSize = m_horizontal ? container.ViewportWidth() : container.ViewportHeight();
    double offsetEnd = offsetStart + viewSize;
    // Do some sanity checks
    if (viewSize == 0 || m_pages.empty()) {
      return;
    }
    // Go through pages untill we reach a visible page
    int page = 0;
    double visiblePagePixels = 0;
    for (; page < (int)m_pages.size() && visiblePagePixels == 0; ++page) {
      visiblePagePixels = m_pages[page].pageVisiblePixels(m_horizontal, offsetStart, offsetEnd);
    }
    if (page == m_pages.size()) {
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
    // Render all visible pages
    auto firstPage = page;
    auto lastPage = page + 1;
    while (firstPage >= 0 && m_pages[firstPage].pageVisiblePixels(m_horizontal, offsetStart, offsetEnd) > 0)
      --firstPage;
    if (firstPage < 0)
      firstPage = 0;
    while (lastPage < m_pages.size() && m_pages[lastPage].pageVisiblePixels(m_horizontal, offsetStart, offsetEnd) > 0)
      ++lastPage;
    if (page != m_currentPage) {
      m_currentPage = page ;
      m_reactContext.DispatchEvent(
        *this,
        L"topPageChanged",
        [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
          eventDataWriter.WriteObjectBegin();
          {
            WriteProperty(eventDataWriter, L"value", page + 1);
          }
          eventDataWriter.WriteObjectEnd();
        });
    }
    SignalError("Current page: " + std::to_string(page + 1) + ". Will render pages " + std::to_string(firstPage + 1) + " to " + std::to_string(lastPage + 1));
    lock.unlock();
    UpdateMultiplePagesRender(firstPage, lastPage);
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

  void RCTPdfControl::SignalError(const std::string& error) {
    m_reactContext.DispatchEvent(
      *this,
      L"topError",
      [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
        eventDataWriter.WriteObjectBegin();
        {
          WriteProperty(eventDataWriter, L"value", winrt::to_hstring(error));
        }
        eventDataWriter.WriteObjectEnd();
      });
  }
}

