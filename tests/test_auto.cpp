#include "../components/tion-api/pi_controller.h"
#include "../components/tion-api/tion-api-4s-internal.h"
#include "../components/tion_4s_uart/tion_4s_uart_vport.h"
#include "../components/tion/tion_component.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

namespace {

using dentra::tion_4s::tion4s_raw_state_set_req_t;
using dentra::tion_4s::tion4s_state_t;
using dentra::tion::TionGatePosition;
using dentra::tion::TionTraits;
using dentra::tion::TionState;
using dentra::tion_4s::Tion4sApi;
using esphome::tion::Tion4sApiComponent;

using Tion4sUartVPort = esphome::tion::Tion4sUartVPort;

using TionUartVPortApi = esphome::tion::TionVPortApi<Tion4sUartIOTest::frame_spec_type, Tion4sApi>;

class Tion4sUartVPortApiTest : public TionUartVPortApi {
 public:
  Tion4sUartVPortApiTest(Tion4sUartVPort *vport) : TionUartVPortApi(vport) {
    this->set_writer(
        Tion4sApi::writer_type::create<Tion4sUartVPortApiTest, &Tion4sUartVPortApiTest::test_write_>(*this));

    // this->set_writer(
    //     [this](uint16_t type, const void *data, size_t size) { return this->test_write_(type, data, size); });

    this->state_.fan_speed = 1;
  }

 protected:
  bool test_write_(uint16_t type, const void *data, size_t size) {
    if (type == dentra::tion_4s::FRAME_TYPE_STATE_SET && size == sizeof(tion4s_raw_state_set_req_t)) {
      const auto *req = static_cast<const tion4s_raw_state_set_req_t *>(data);
      this->state_.auto_state = req->data.ma_connected;
      this->on_state_fn.call_if(this->state_, req->request_id);
    }

    return true;
    // return this->write_frame_(type, data, size);
  }
};

#define ONE_SECOND 1000
#define ONE_MINUTE (ONE_SECOND * 60)

bool test_auto_pi() {
  bool res = true;

  size_t api = 1;

  auto make_call = [](size_t *api, int **pp_call) -> int * {
    if (*pp_call == nullptr) {
      *pp_call = new int(*api);
    }
    return *pp_call;
  };

  auto make_call2 = [api](int **pp_call) -> int * {
    if (*pp_call == nullptr) {
      *pp_call = new int(api);
    }
    return *pp_call;
  };

  int *call{};

  auto make_call3 = [api, pp_call = &call]() -> int * {
    if (*pp_call == nullptr) {
      *pp_call = new int(api);
    }
    return *pp_call;
  };

  printf("%p\n", make_call(&api, &call));
  printf("%p\n", make_call(&api, &call));
  printf("%p\n", make_call2(&call));
  printf("%p\n", make_call3());
  printf("%p\n", call);
  printf("%zu\n", sizeof(make_call));
  printf("%zu\n", sizeof(make_call2));
  printf("%zu\n", sizeof(make_call3));
  return false;

  dentra::tion::auto_co2::PIController pi(0.2736, 8, 50, 30, 60);

  uint16_t spa[] = {450, 470, 510, 600, 680, 720, 780, 810, 770, 740, 710, 690, 650, 630, 610};

  for (auto &&sp : spa) {
    auto rate = pi.update(700, sp);
    ESP_LOGD(TAG, "sp=%d, rate=%f, %d", sp, rate, int(rate + .5));
    esphome::test_set_millis(esphome::millis() + 10 * ONE_MINUTE);
  }

  for (int i = 0; i < 100; i++) {
    const int sp = 610;
    auto rate = pi.update(700, sp);
    ESP_LOGD(TAG, "sp=%d, rate=%f, %d", sp, rate, int(rate + .5));
    esphome::test_set_millis(esphome::millis() + 10 * ONE_MINUTE);
  }

  return res;
}

bool test_auto() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sApiComponent capi(&api, vport.get_type());

  capi.set_batch_timeout(3000);
  cloak::setup_and_loop({&vport});

  capi.api()->set_auto_pi_data(0.2736, 8, 50);

  capi.api()->set_auto_setpoint(700);
  capi.api()->set_auto_min_fan_speed(1);
  capi.api()->set_auto_max_fan_speed(3);

  auto *call = capi.make_call();
  call->set_auto_state(true);
  call->perform();

  uint16_t spa[] = {450, 470, 510, 600, 680, 720, 780, 810, 770, 740, 710, 690};

  for (auto &&sp : spa) {
    capi.api()->auto_update(sp, call);
    call->perform();
    esphome::test_set_millis(esphome::millis() + 10 * ONE_MINUTE);
  }

  for (int i = 0; i < 100; i++) {
    capi.api()->auto_update(650, call);
    call->perform();
    esphome::test_set_millis(esphome::millis() + 10 * ONE_MINUTE);
  }

  return res;
}

REGISTER_TEST(test_auto);
REGISTER_TEST(test_auto_pi);

}  // namespace
