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
        /*m_textChangedRevoker = TextElement().TextChanged(winrt::auto_revoke,
          [ref = get_weak()](auto const& sender, auto const& args) {
          if (auto self = ref.get()) {
            self->OnTextChanged(sender, args);
          }
        });*/
    }

    void RCTPdfControl::OnTextChanged(winrt::Windows::Foundation::IInspectable const&,
      winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs const&) {
      // TODO: example sending event on text changed
      /*auto text = TextElement().Text();
      m_reactContext.DispatchEvent(
        *this,
        L"sampleEvent",
        [&](winrt::Microsoft::ReactNative::IJSValueWriter const& eventDataWriter) noexcept {
          eventDataWriter.WriteObjectBegin();
          WriteProperty(eventDataWriter, L"text", text);
          eventDataWriter.WriteObjectEnd();
        }
      );*/
    }

    winrt::Windows::Foundation::Collections::
      IMapView<winrt::hstring, winrt::Microsoft::ReactNative::ViewManagerPropertyType>
      RCTPdfControl::NativeProps() noexcept {
      // TODO: define props here
      auto nativeProps = winrt::single_threaded_map<hstring, ViewManagerPropertyType>();
      nativeProps.Insert(L"path", ViewManagerPropertyType::String);
      return nativeProps.GetView();
    }

    void RCTPdfControl::UpdateProperties(winrt::Microsoft::ReactNative::IJSValueReader const& propertyMapReader) noexcept {
      // TODO: handle the props here
      const JSValueObject& propertyMap = JSValue::ReadObjectFrom(propertyMapReader);
      for (auto const& pair : propertyMap) {
        auto const& propertyName = pair.first;
        auto const& propertyValue = pair.second;
        if (propertyName == "path") {
          if (propertyValue != nullptr) {
            auto const& value = propertyValue.AsString();
            loadPDF(value, "");
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
        // TODO: define events emitted by the control
        WriteCustomDirectEventTypeConstant(constantWriter, "sampleEvent");
      };
    }

    winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> RCTPdfControl::Commands() noexcept {
      // TODO: deifne commands supported by the control
      auto commands = winrt::single_threaded_vector<hstring>();
      commands.Append(L"sampleCommand");
      return commands.GetView();
    }

    void RCTPdfControl::DispatchCommand(winrt::hstring const& commandId, winrt::Microsoft::ReactNative::IJSValueReader const& commandArgsReader) noexcept {
      // TODO: handle commands here
      auto commandArgs = JSValue::ReadArrayFrom(commandArgsReader);
      if (commandId == L"sampleCommand") {
        //TextElement().Text(L"sampleCommand used!");
      }
    }

    winrt::fire_and_forget RCTPdfControl::loadPDF(std::string filename, std::string password) {
      auto lifetime = get_strong();
      //auto file = co_await StorageFile::GetFileFromPathAsync(winrt::to_hstring(filename));
      FileOpenPicker picker;
      picker.FileTypeFilter().Append(L".pdf");
      StorageFile file = co_await picker.PickSingleFileAsync();
      auto document = co_await PdfDocument::LoadFromFileAsync(file, winrt::to_hstring(password));
      auto items = Pages().Items();
      items.Clear();
      for (unsigned pageIdx = 0; pageIdx < document.PageCount(); ++pageIdx) {
        auto page = document.GetPage(pageIdx);
        auto dims = page.Size();
        InMemoryRandomAccessStream stream;
        PdfPageRenderOptions renderOptions;
        renderOptions.DestinationHeight(dims.Height * 0.3);
        renderOptions.DestinationWidth(dims.Width * 0.3);
        co_await page.RenderToStreamAsync(stream, renderOptions);
        BitmapImage image;
        co_await image.SetSourceAsync(stream);
        Image pageImage;
        pageImage.Source(image);
        pageImage.HorizontalAlignment(HorizontalAlignment::Center);
        pageImage.Margin(ThicknessHelper::FromLengths(0, 10, 0, 10));
        //pageImage.MinWidth(dims.Width*0.5);
        //pageImage.MinHeight(dims.Height*0.5);
        pageImage.Width(dims.Width * 0.3);
        pageImage.Height(dims.Height * 0.3);
        //pageImage.MaxWidth(dims.Width * 0.5);
        //pageImage.MaxHeight(dims.Height * 0.5);
        items.Append(pageImage);
      }
      //Pages().Width(1060);
      //Pages().Scale(Numerics::float3(0.5));
    }

}
