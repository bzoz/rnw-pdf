﻿#include "pch.h"
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
      // TODO: nativeProps.Insert(L"page", ViewManagerPropertyType::Number);
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
      // TODO: handle the props here
      const JSValueObject& propertyMap = JSValue::ReadObjectFrom(propertyMapReader);
      for (auto const& pair : propertyMap) {
        auto const& propertyName = pair.first;
        auto const& propertyValue = pair.second;
        if (propertyName == "path" || propertyName == "password") {
          if (propertyValue != nullptr) {
            auto const& value = propertyValue.AsString();
            if (auto password = propertyMap.find("password"); password != propertyMap.end()) {
              loadPDF(value, password->second.AsString());
            } else {
              loadPDF(value, {});
            }
            //TextElement().Text(winrt::to_hstring(value));
          }
          else {
            //TextElement().Text(L"");
          }
        }
      }
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

    winrt::fire_and_forget RCTPdfControl::loadPDF(std::string pdfURI, std::string pdfPassword) {
      if (m_pdfURI == pdfURI && m_pdfPassword == pdfPassword) {
        return;
      }
      m_pdfURI = pdfURI;
      m_pdfPassword = pdfPassword;
      auto lifetime = get_strong();
      auto uri = Uri(winrt::to_hstring(pdfURI));
      auto file = co_await StorageFile::GetFileFromApplicationUriAsync(uri);
      auto document = co_await PdfDocument::LoadFromFileAsync(file, winrt::to_hstring(pdfPassword));
      auto items = Pages().Items();
      items.Clear();
      m_currentPage = 0;
      m_pageWidth.clear();
      m_pageHeight.clear();
      for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
        auto page = document.GetPage(pageIdx);
        auto dims = page.Size();
        m_pageWidth.push_back(dims.Width);
        m_pageHeight.push_back(dims.Height);
        InMemoryRandomAccessStream stream;
        PdfPageRenderOptions renderOptions;
        renderOptions.DestinationHeight(dims.Height * m_renderedScale);
        renderOptions.DestinationWidth(dims.Width * m_renderedScale);
        co_await page.RenderToStreamAsync(stream, renderOptions);
        BitmapImage image;
        co_await image.SetSourceAsync(stream);
        Image pageImage;
        pageImage.Source(image);
        pageImage.HorizontalAlignment(HorizontalAlignment::Center);
        pageImage.Margin(ThicknessHelper::FromUniformLength(m_margins * m_displayScale));
        pageImage.Width(dims.Width * m_displayScale);
        pageImage.Height(dims.Height * m_displayScale);
        items.Append(pageImage);
      }
      GoToPage(1);
      SignalError("Page loaded!");
    }

    void RCTPdfControl::OnViewChanged(winrt::Windows::Foundation::IInspectable const& sender,
      winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const& args) {
      // TODO cache the scaled page sizes
      auto container = PagesContainer();
      double offset = m_horizontal ? container.HorizontalOffset() : container.VerticalOffset();
      SignalError(std::to_string(offset) + " of " + std::to_string(PagesContainer().ScrollableHeight()));
      double viewSize = m_horizontal ? container.ViewportWidth() : container.ViewportHeight();
      const auto& pageSize = m_horizontal ? m_pageWidth : m_pageHeight;
      // Do some sanity checks
      if (viewSize == 0 || pageSize.empty()) {
        return; 
      }
      // Sum pages size until we reach our offset
      int page = 0;
      double pageEndOffset = 0;
      while (page < pageSize.size() && pageEndOffset < offset) {
        // Make sure we include both bottom and top margins
        pageEndOffset += (pageSize[page++] + m_margins * 2) * m_displayScale;
      }
      if (page > 0)
        --page;
      // Bottom border of page #"page" is visible. Check how much of the view port this page covers...
      double pageVisiblePixels = (pageSize[page] + m_margins * 2) * m_displayScale - offset;
      double viewCoveredByPage = pageVisiblePixels / viewSize;
      // ...and how much of the page is visible:
      double pageVisiblePart = pageVisiblePixels / ((pageSize[page] + m_margins * 2) * m_displayScale);
      // If:
      //  - less than 50% of the screen is covered by the page
      //  - less than 50% of the page is visible (important if more than one page fits the screen)
      //  - there is a next page
      // move the indicator to that page:
      if (viewCoveredByPage < 0.5 && pageVisiblePart < 0.5 && page + 1 < pageSize.size()) {
        ++page;
      }
      // TODO: how to trigger events?
      if (page != m_currentPage) {
        m_currentPage = page;
      }
    }

    void RCTPdfControl::GoToPage(int page) {
      if (page < 0 || page > m_pageHeight.size()) {
        return;
      }
      const auto& pageSize = m_horizontal ? m_pageWidth : m_pageHeight;
      double neededOffset = 0;
      for (int pageIdx = 0; pageIdx < page; ++pageIdx) {
        neededOffset += pageSize[pageIdx] + m_margins * 2;
      }
      neededOffset *= m_displayScale;
      //neededOffset -= 500;
      SignalError("Going to " + std::to_string(neededOffset));
      SignalError("Max: " + std::to_string(PagesContainer().ScrollableHeight()));
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

