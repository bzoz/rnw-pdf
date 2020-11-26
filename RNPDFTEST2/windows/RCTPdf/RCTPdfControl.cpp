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
      std::unique_lock lock(m_renderLock);
      const JSValueObject& propertyMap = JSValue::ReadObjectFrom(propertyMapReader);
      m_pdfURI.clear();
      m_pdfPassword.clear();
      m_currentPage = 0;
      for (auto const& pair : propertyMap) {
        auto const& propertyName = pair.first;
        auto const& propertyValue = pair.second;
        if (propertyName == "path") {
          m_pdfURI = propertyValue != nullptr ? propertyValue.AsString() : "";
        } else if (propertyName == "password") {
          m_pdfPassword = propertyValue != nullptr ? propertyValue.AsString() : "";
        } else if (propertyName == "page") {
          m_currentPage = propertyValue != nullptr ? propertyValue.AsInt32() - 1 : 0;
        }
      
      }
      loadPDF(std::move(lock));
    }

    winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomBubblingEventTypeConstants() noexcept {
      return nullptr;
    }

    winrt::Microsoft::ReactNative::ConstantProviderDelegate RCTPdfControl::ExportedCustomDirectEventTypeConstants() noexcept {
      return [](IJSValueWriter const& constantWriter) {
        // TODO: WriteCustomDirectEventTypeConstant(constantWriter, "onLoadComplete");
        // TODO: WriteCustomDirectEventTypeConstant(constantWriter, "onPageChanged");
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

    static winrt::IAsyncOperation<BitmapImage> renderPDFPage(PdfPage& page, double scale) {
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

    winrt::fire_and_forget RCTPdfControl::loadPDF(std::unique_lock<std::mutex> lock) {
      auto lifetime = get_strong();
      auto uri = Uri(winrt::to_hstring(m_pdfURI));
      auto file = co_await StorageFile::GetFileFromApplicationUriAsync(uri);
      auto document = co_await PdfDocument::LoadFromFileAsync(file, winrt::to_hstring(m_pdfPassword));
      auto items = Pages().Items();
      items.Clear();
      m_pageWidth.clear();
      m_pageHeight.clear();
      m_pages.clear();
      double totalHeigh = 0;
      double targetOffset = 0;
      for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
        if (pageIdx == m_currentPage) {
          targetOffset = totalHeigh;
        }
        auto page = document.GetPage(pageIdx);
        auto dims = page.Size();
        totalHeigh += dims.Height + 2*m_margins; 
      }
      for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
        auto page = document.GetPage(pageIdx);
        auto dims = page.Size();
        m_pageWidth.push_back(dims.Width);
        m_pageHeight.push_back(dims.Height);
        /*InMemoryRandomAccessStream stream;
        PdfPageRenderOptions renderOptions;
        renderOptions.DestinationHeight(static_cast<uint32_t>(dims.Height * m_scale));
        renderOptions.DestinationWidth(static_cast<uint32_t>(dims.Width * m_scale));
        co_await page.RenderToStreamAsync(stream, renderOptions);
        BitmapImage image;
        co_await image.SetSourceAsync(stream);*/
        Image pageImage;
        //pageImage.Source(image);
        pageImage.HorizontalAlignment(HorizontalAlignment::Center);
        pageImage.Margin(ThicknessHelper::FromUniformLength(m_margins * m_scale));
        pageImage.Width(dims.Width * m_scale);
        pageImage.Height(dims.Height * m_scale);
        items.Append(pageImage);
        m_pages.push_back(pageImage);
      }
      m_pages[m_currentPage].Source(co_await renderPDFPage(document.GetPage(m_currentPage), m_scale));
      PagesContainer().ChangeView(0.0, targetOffset, nullptr, true);
      for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
        auto page = document.GetPage(pageIdx);
        auto dims = page.Size();
        InMemoryRandomAccessStream stream;
        PdfPageRenderOptions renderOptions;
        renderOptions.DestinationHeight(static_cast<uint32_t>(dims.Height * m_scale));
        renderOptions.DestinationWidth(static_cast<uint32_t>(dims.Width * m_scale));
        co_await page.RenderToStreamAsync(stream, renderOptions);
        BitmapImage image;
        co_await image.SetSourceAsync(stream);
        m_pages[pageIdx].Source(image);
      }
      lock.unlock();
    }

    void RCTPdfControl::OnViewChanged(winrt::Windows::Foundation::IInspectable const&,
      winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const&) {
      // TODO cache the scaled page sizes
      auto container = PagesContainer();
      double offset = m_horizontal ? container.HorizontalOffset() : container.VerticalOffset();
      if (m_targetOffset != -1 && offset != m_targetOffset) {
        // continue scrooling until we reach the target
        double horizontalOffset = m_horizontal ? m_targetOffset : PagesContainer().HorizontalOffset();
        double verticalOffset = m_horizontal ? PagesContainer().VerticalOffset() : m_targetOffset;
        m_targetOffset = -1; // do this only once
        PagesContainer().ChangeView(horizontalOffset, verticalOffset, nullptr, true);
        return;
      }
      double viewSize = m_horizontal ? container.ViewportWidth() : container.ViewportHeight();
      const auto& pageSize = m_horizontal ? m_pageWidth : m_pageHeight;
      // Do some sanity checks
      if (viewSize == 0 || pageSize.empty()) {
        return; 
      }
      // Sum pages size until we reach our offset
      int page = 0;
      double pageEndOffset = 0;
      while (static_cast<std::size_t>(page) < pageSize.size() && pageEndOffset < offset) {
        // Make sure we include both bottom and top margins
        pageEndOffset += (pageSize[page++] + m_margins * 2) * m_scale;
      }
      if (page > 0)
        --page;
      // Bottom border of page #"page" is visible. Check how much of the view port this page covers...
      double pageVisiblePixels = (pageSize[page] + m_margins * 2) * m_scale - offset;
      double viewCoveredByPage = pageVisiblePixels / viewSize;
      // ...and how much of the page is visible:
      double pageVisiblePart = pageVisiblePixels / ((pageSize[page] + m_margins * 2) * m_scale);
      // If:
      //  - less than 50% of the screen is covered by the page
      //  - less than 50% of the page is visible (important if more than one page fits the screen)
      //  - there is a next page
      // move the indicator to that page:
      if (viewCoveredByPage < 0.5 &&
          pageVisiblePart < 0.5 &&
          static_cast<std::size_t>(page + 1) < pageSize.size()) {
        ++page;
      }
      // TODO: how to trigger events?
      if (page != m_currentPage) {
        m_currentPage = page;
      }
    }

    void RCTPdfControl::GoToPage(int page) {
      std::scoped_lock lock(m_renderLock);
      if (page < 0 || static_cast<std::size_t>(page) > m_pageHeight.size()) {
        return;
      }
      const auto& pageSize = m_horizontal ? m_pageWidth : m_pageHeight;
      double neededOffset = 0;
      for (int pageIdx = 0; pageIdx < page; ++pageIdx) {
        neededOffset += pageSize[pageIdx] + m_margins * 2;
      }
      neededOffset *= m_scale;
      //neededOffset -= 500;
      SignalError("Going to " + std::to_string(neededOffset));
      SignalError("Max: " + std::to_string(PagesContainer().ScrollableHeight()));
      double horizontalOffset = m_horizontal ? neededOffset : PagesContainer().HorizontalOffset();
      double verticalOffset = m_horizontal ? PagesContainer().VerticalOffset() : neededOffset;
      m_targetOffset = neededOffset;
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

