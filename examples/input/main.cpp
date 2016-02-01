#include <fstream>
#include <iostream>
#include <unordered_map>

#include <format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <rxcpp/rx.hpp>

#include <rxcpp/rx-subjects.hpp>

#include <autograph/backend/opengl/backend.hpp>
#include <autograph/device.hpp>
#include <autograph/draw.hpp>
#include <autograph/pipeline.hpp>
#include <autograph/pixel_format.hpp>
#include <autograph/surface.hpp>

#include <extra/image_io/load_image.hpp>

#include "../common/sample.hpp"
#include "../common/uniforms.hpp"

#include <Windows.h>
#include <ole2.h>
#include <rtscom.h>
#include <rtscom_i.c>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifdef _WIN64
// Fix for NEXTRAWINPUTBLOCK (QWORD macro not defined)
// Yes indeed, with winapi, we need to fill in the blanks ourselves.
// I guess the programmer feels more involved that way.
typedef unsigned __int64 QWORD;
#endif

// stylus event handler
class CRTSEventHandler : public IStylusSyncPlugin {
public:
  CRTSEventHandler() : m_nRef(1), m_pPunkFTMarshaller(NULL) {}
  virtual ~CRTSEventHandler() {
    if (m_pPunkFTMarshaller != NULL)
      m_pPunkFTMarshaller->Release();
  }

  // IStylusSyncPlugin inherited methods

  // Methods whose data we use
  STDMETHOD(Packets)
  (IRealTimeStylus* pStylus, const StylusInfo* pStylusInfo, ULONG nPackets,
   ULONG nPacketBuf, LONG* pPackets, ULONG* nOutPackets, LONG** ppOutPackets) {
    fmt::print("Packets!\n");
    return S_OK;
  }
  STDMETHOD(InAirPackets)
  (IRealTimeStylus* pStylus, const StylusInfo* pStylusInfo, ULONG nPackets,
   ULONG nPacketBuf, LONG* pPackets, ULONG* nOutPackets, LONG** ppOutPackets) {
    fmt::print("In air packet!\n");
    return S_OK;
  }
  STDMETHOD(DataInterest)(RealTimeStylusDataInterest* pEventInterest) {
    *pEventInterest =
        (RealTimeStylusDataInterest)(RTSDI_Packets | RTSDI_InAirPackets);
    return S_OK;
  }

  // Methods you can add if you need the alerts - don't forget to change
  // DataInterest!
  STDMETHOD(StylusDown)
  (IRealTimeStylus*, const StylusInfo*, ULONG, LONG* _pPackets, LONG**) {
    return S_OK;
  }
  STDMETHOD(StylusUp)
  (IRealTimeStylus*, const StylusInfo*, ULONG, LONG* _pPackets, LONG**) {
    return S_OK;
  }
  STDMETHOD(RealTimeStylusEnabled)
  (IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) { return S_OK; }
  STDMETHOD(RealTimeStylusDisabled)
  (IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) { return S_OK; }
  STDMETHOD(StylusInRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) {
    return S_OK;
  }
  STDMETHOD(StylusOutOfRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) {
    return S_OK;
  }
  /*STDMETHOD(InAirPackets)
  (IRealTimeStylus*, const StylusInfo*, ULONG, ULONG, LONG*, ULONG*, LONG**) {
    return S_OK;
  }*/
  STDMETHOD(StylusButtonUp)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) {
    return S_OK;
  }
  STDMETHOD(StylusButtonDown)
  (IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) { return S_OK; }
  STDMETHOD(SystemEvent)
  (IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT,
   SYSTEM_EVENT_DATA) {
    return S_OK;
  }
  STDMETHOD(TabletAdded)(IRealTimeStylus*, IInkTablet*) { return S_OK; }
  STDMETHOD(TabletRemoved)(IRealTimeStylus*, LONG) { return S_OK; }
  STDMETHOD(CustomStylusDataAdded)
  (IRealTimeStylus*, const GUID*, ULONG, const BYTE*) { return S_OK; }
  STDMETHOD(Error)
  (IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT,
   LONG_PTR*) {
    return S_OK;
  }
  STDMETHOD(UpdateMapping)(IRealTimeStylus*) { return S_OK; }

  // IUnknown methods
  STDMETHOD_(ULONG, AddRef)() { return InterlockedIncrement(&m_nRef); }
  STDMETHOD_(ULONG, Release)() {
    ULONG nNewRef = InterlockedDecrement(&m_nRef);
    if (nNewRef == 0)
      delete this;

    return nNewRef;
  }
  STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) {
    if ((riid == IID_IStylusSyncPlugin) || (riid == IID_IUnknown)) {
      *ppvObj = this;
      AddRef();
      return S_OK;
    } else if ((riid == IID_IMarshal) && (m_pPunkFTMarshaller != NULL)) {
      return m_pPunkFTMarshaller->QueryInterface(riid, ppvObj);
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
  }

  LONG m_nRef;                   // COM object reference count
  IUnknown* m_pPunkFTMarshaller; // free-threaded marshaller
};

using GL = ag::opengl::OpenGLBackend;

// KeyEvent: (key, status)
// CursorEvent: (position, inside)
// MouseButtonEvent: (button)
// TabletEvent: (device info ptr, absolute position, pressure, tilt) (includes
// mouse cursor)
// ScrollEvent: (delta)

// TabletDeviceInfo: name, size, resolution, capabilities

unsigned frame_call = 0;

struct GLFWInputBackend {
  GLFWInputBackend(GLFWwindow* window_)
      : window(window_), subject_keys(0), subject_mouse_buttons(0),
        subject_mouse_pos(glm::vec2()), sub_keys(subject_keys.get_subscriber()),
        sub_mouse_buttons(subject_mouse_buttons.get_subscriber()),
        sub_mouse_pos(subject_mouse_pos.get_subscriber()) {
    glfwSetCharCallback(window, GLFWCharHandler);
    glfwSetCursorEnterCallback(window, GLFWCursorEnterHandler);
    glfwSetCursorPosCallback(window, GLFWCursorPosHandler);
    glfwSetKeyCallback(window, GLFWKeyHandler);
    glfwSetMouseButtonCallback(window, GLFWMouseButtonHandler);
    glfwSetScrollCallback(window, GLFWScrollHandler);
    native_handle = glfwGetWin32Window(window);
    obs_keys = subject_keys.get_observable();
    obs_mouse_buttons = subject_mouse_buttons.get_observable();
    obs_mouse_pos = subject_mouse_pos.get_observable();
    instance = this;
    createStylus();
    // glfwSwapInterval(1);
  }

  /* void initRawInput() {
           RAWINPUTDEVICE devices[] = {
                   { 0x01  , 0x06 , 0  , NULL },
                   { 0x01 , 0x02 , 0  , NULL },
           };
     if (RegisterRawInputDevices(devices, 2, sizeof(devices[0])) == FALSE)
       ag::failWith("RegisterRawInputDevices");
   }

   void readRawInput() {
     UINT cbSize;
     //Sleep(1000);

     GetRawInputBuffer(NULL, &cbSize, sizeof(RAWINPUTHEADER));
         if (cbSize == 0)
                 return;
     // Quoted from the official MSDN documentation:
     // "this is a wild guess"
     cbSize*=2048;
     auto rawInputBuffer = std::make_unique<uint8_t[]>(cbSize);

     PRAWINPUT pRawInput = (PRAWINPUT)rawInputBuffer.get();
         int nInput = 0;

         while (true)
         {
                 nInput = GetRawInputBuffer(pRawInput, &cbSize,
   sizeof(RAWINPUTHEADER));
                 if (nInput == 0 )
                         break;
                 if (nInput < 0)
                         ag::failWith("GetRawInputBuffer error");
                 std::vector<PRAWINPUT> rawInputEntries;
                 rawInputEntries.reserve(nInput);

                 PRAWINPUT pri = pRawInput;
                 for (UINT i = 0; i < nInput; ++i) {
                         fmt::print("input[{}] = {}\n", i, (const void*)pri);
                         rawInputEntries.push_back(pri);
                         pri = NEXTRAWINPUTBLOCK(pri);
                 }

                 DefRawInputProc(rawInputEntries.data(), nInput,
   sizeof(RAWINPUTHEADER));
         }

   }*/

  void createStylus() {
    HRESULT hr = CoCreateInstance(CLSID_RealTimeStylus, NULL, CLSCTX_ALL,
                                  IID_PPV_ARGS(&stylus));
    if (FAILED(hr))
      ag::failWith(fmt::format("CoCreateInstance failed (HRESULT {})", hr));

    hr = stylus->put_HWND((HANDLE_PTR)native_handle);
    if (FAILED(hr))
      ag::failWith(fmt::format("put_HWND failed (HRESULT {})", hr));

    // Create eventhandler
    stylus_event_handler = std::make_unique<CRTSEventHandler>();

    // Create free-threaded marshaller for this object and aggregate it.
    hr = CoCreateFreeThreadedMarshaler(
        stylus_event_handler.get(), &stylus_event_handler->m_pPunkFTMarshaller);
    if (FAILED(hr))
      ag::failWith(
          fmt::format("CoCreateFreeThreadedMarshaler failed (HRESULT {})", hr));

    // Add handler object to the list of synchronous plugins in the RTS object.
    hr = stylus->AddStylusSyncPlugin(0, stylus_event_handler.get());
    if (FAILED(hr))
      ag::failWith(fmt::format("AddStylusSyncPlugin failed (HRESULT {})", hr));

    // Set data we want - we're not actually using all of this, but we're gonna
    // get X and Y anyway so might as well set it

    GUID wanted_props[] = {GUID_PACKETPROPERTY_GUID_X,
                           GUID_PACKETPROPERTY_GUID_Y,
                           GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE,
                           GUID_PACKETPROPERTY_GUID_X_TILT_ORIENTATION};
    stylus->SetDesiredPacketDescription(4, wanted_props);
    stylus->put_Enabled(true);

    std::vector<TABLET_CONTEXT_ID> tabletContexts;

    // Detect what tablet context IDs will give us pressure data
    {
      // g_nContexts = 0;
      ULONG nTabletContexts = 0;
      TABLET_CONTEXT_ID* piTabletContexts;
      HRESULT res =
          stylus->GetAllTabletContextIds(&nTabletContexts, &piTabletContexts);
      for (ULONG i = 0; i < nTabletContexts; i++) {
        IInkTablet* pInkTablet;
        if (SUCCEEDED(stylus->GetTabletFromTabletContextId(piTabletContexts[i],
                                                           &pInkTablet))) {
          float ScaleX, ScaleY;
          ULONG nPacketProps;
          PACKET_PROPERTY* pPacketProps;
          BSTR tabletName;
          pInkTablet->get_Name(&tabletName);
          std::wcout << fmt::format(L"Tablet Name: {}\n",
                                    (const wchar_t*)tabletName);
          stylus->GetPacketDescriptionData(piTabletContexts[i], &ScaleX,
                                           &ScaleY, &nPacketProps,
                                           &pPacketProps);

          for (ULONG j = 0; j < nPacketProps; j++) {
            fmt::print("\tProperty #{}, metrics {} {} {} {}\n", j,
                       pPacketProps[j].PropertyMetrics.fResolution,
                       pPacketProps[j].PropertyMetrics.nLogicalMin,
                       pPacketProps[j].PropertyMetrics.nLogicalMax,
                       pPacketProps[j].PropertyMetrics.Units);
            if (pPacketProps[j].guid !=
                GUID_PACKETPROPERTY_GUID_NORMAL_PRESSURE)
              continue;

            /*g_lContexts[g_nContexts].iTabletContext = piTabletContexts[i];
            g_lContexts[g_nContexts].iPressure = j;
            g_lContexts[g_nContexts].PressureRcp = 1.0f /
    pPacketProps[j].PropertyMetrics.nLogicalMax;
            g_nContexts++;
            break;*/
          }
          CoTaskMemFree(pPacketProps);
        }
      }

      // If we can't get pressure information, no use in having the tablet
      // context
      /*if (g_nContexts == 0)
      {
              ReleaseRTS();
              return false;
      }*/
    }
  }

  auto keys() { return obs_keys; }

  auto mouse_pos() { return obs_mouse_pos; }

  auto mouse_buttons() { return obs_mouse_buttons; }

  void poll() { glfwPollEvents(); }

  auto key_state(int key) {
	  // not sure how that is supposed to work, but it does!
	  // probably some shared_ptr magic
    auto b = rxcpp::subjects::behavior<int>(0);
	obs_keys.filter([=](auto k) { return k == key; }).subscribe(b.get_subscriber());
	return b;
  }

  void on_mouse_button(GLFWwindow* window, int button, int action, int mods) {
    sub_mouse_buttons.on_next(button);
  }

  void on_cursor_pos(GLFWwindow* window, double xpos, double ypos) {
    sub_mouse_pos.on_next(glm::uvec2((unsigned)xpos, (unsigned)ypos));
  }

  void on_cursor_enter(GLFWwindow* window, int entered) {}

  void on_scroll(GLFWwindow* window, double xoffset, double yoffset) {}

  void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    sub_keys.on_next(key);
  }

  void on_char(GLFWwindow* window, unsigned int codepoint) {}

private:
  rxcpp::rxsub::behavior<int> subject_keys;
  rxcpp::rxsub::behavior<int> subject_mouse_buttons;
  rxcpp::rxsub::behavior<glm::uvec2> subject_mouse_pos;

  rxcpp::subscriber<int> sub_keys;
  rxcpp::subscriber<int> sub_mouse_buttons;
  rxcpp::subscriber<glm::uvec2> sub_mouse_pos;

  rxcpp::observable<int> obs_keys;
  rxcpp::observable<int> obs_mouse_buttons;
  rxcpp::observable<glm::uvec2> obs_mouse_pos;

  GLFWwindow* window;
  HWND native_handle;

  IRealTimeStylus* stylus;
  std::unique_ptr<CRTSEventHandler> stylus_event_handler;

  static GLFWInputBackend* instance;

  // GLFW event handlers
  static void GLFWMouseButtonHandler(GLFWwindow* window, int button, int action,
                                     int mods) {
    instance->on_mouse_button(window, button, action, mods);
  }

  static void GLFWCursorPosHandler(GLFWwindow* window, double xpos,
                                   double ypos) {
    frame_call++;
    // fmt::print("Cursor events in frame {}\n", frame_call);
    instance->on_cursor_pos(window, xpos, ypos);
  }

  static void GLFWCursorEnterHandler(GLFWwindow* window, int entered) {
    instance->on_cursor_enter(window, entered);
  }

  static void GLFWScrollHandler(GLFWwindow* window, double xoffset,
                                double yoffset) {
    instance->on_scroll(window, xoffset, yoffset);
  }

  static void GLFWKeyHandler(GLFWwindow* window, int key, int scancode,
                             int action, int mods) {
    instance->on_key(window, key, scancode, action, mods);
  }

  static void GLFWCharHandler(GLFWwindow* window, unsigned int codepoint) {
    instance->on_char(window, codepoint);
  }

  static void GLFWErrorCallback(int error, const char* description) {}
};

GLFWInputBackend* GLFWInputBackend::instance = nullptr;

class InputSample : public samples::GLSample<InputSample> {
public:
  InputSample(unsigned width, unsigned height)
      : GLSample(width, height, "Input"), last_key(0)
	  {
    texDefault = loadTexture2D("common/img/tonberry.jpg");
    input = std::make_unique<GLFWInputBackend>(gl.getWindow());
    the_a_key = input->keys().filter([](auto k) { return k == GLFW_KEY_A; });
    the_a_key.subscribe([](auto k) { fmt::print("Pressed A!\n"); });
    input->mouse_pos().subscribe(
        [](auto v) { fmt::print("Mouse pos: {},{}\n", v.x, v.y); });
    last_key = input->key_state(GLFW_KEY_3);
    // fmt::print("Mouse pos: {} {}", input->mouse_pos().;
  }

  void render() {
    using namespace glm;
    samples::uniforms::Object objectData;
    auto out = device->getOutputSurface();
    ag::clear(*device, out, ag::ClearColor{1.0f, 0.0f, 1.0f, 1.0f});
    copyTex(texDefault, out, width, height, {20, 20}, 1.0);
    frame_call = 0;
    fmt::print("Last key is: {}\n", last_key.get_value());
  }

private:
  rxcpp::observable<int> the_a_key;
  rxcpp::subjects::behavior<int> last_key;
  std::unique_ptr<GLFWInputBackend> input;
  ag::Texture2D<ag::RGBA8, GL> texDefault;
};

int main() {
  InputSample sample(1000, 800);
  return sample.run();
}
