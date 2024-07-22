#include <cstdio>
#include <climits>

#include "utils.h"

#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-ble-3s.h"
#include "../components/tion-api/tion-api-4s-internal.h"

DEFINE_TAG;

#define BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME 0x09 /**< Complete local device name. */

/**@defgroup BLE_GAP_ADV_TYPES GAP Advertising types
 * @{ */
#define BLE_GAP_ADV_TYPE_ADV_IND 0x00         /**< Connectable undirected. */
#define BLE_GAP_ADV_TYPE_ADV_DIRECT_IND 0x01  /**< Connectable directed. */
#define BLE_GAP_ADV_TYPE_ADV_SCAN_IND 0x02    /**< Scannable undirected. */
#define BLE_GAP_ADV_TYPE_ADV_NONCONN_IND 0x03 /**< Non connectable undirected. */
/**@} */

#define BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA 0xFF /**< Manufacturer Specific Data. */

/**@brief Maximum size of advertising data in octets. */
#define BLE_GAP_ADV_MAX_SIZE (31)

/** @brief BLE address length. */
#define BLE_GAP_ADDR_LEN (6)

/**@brief Bluetooth Low Energy address. */
typedef struct {
  uint8_t addr_id_peer : 1;       /**< Only valid for peer addresses.
                                       Reference to peer in device identities list (as set with @ref
                                     sd_ble_gap_device_identities_set) when peer is using privacy. */
  uint8_t addr_type : 7;          /**< See @ref BLE_GAP_ADDR_TYPES. */
  uint8_t addr[BLE_GAP_ADDR_LEN]; /**< 48-bit address, LSB format. */
} ble_gap_addr_t;

/**@brief Event structure for @ref BLE_GAP_EVT_ADV_REPORT. */
typedef struct {
  ble_gap_addr_t peer_addr; /**< Bluetooth address of the peer device. */
  int8_t rssi;              /**< Received Signal Strength Indication in dBm. */
  uint8_t scan_rsp : 1;     /**< If 1, the report corresponds to a scan response and the type field may be ignored. */
  uint8_t type : 2;         /**< See @ref BLE_GAP_ADV_TYPES. Only valid if the scan_rsp field is 0. */
  uint8_t dlen : 5;         /**< Advertising or scan response data length. */
  uint8_t data[BLE_GAP_ADV_MAX_SIZE]; /**< Advertising or scan response data. */
} ble_gap_evt_adv_report_t;

#define NRF_ERROR_BASE_NUM (0x0)                      ///< Global error base
#define NRF_SUCCESS (NRF_ERROR_BASE_NUM + 0)          ///< Successful command
#define NRF_ERROR_NOT_FOUND (NRF_ERROR_BASE_NUM + 5)  ///< Not found

#define PERIPHERAL_NAME "Tion Breezer 3S"

static bool is_name_present(const char *target_name, const ble_gap_evt_adv_report_t *p_adv_report) {
  uint32_t index = 0;
  uint8_t *p_data = (uint8_t *) p_adv_report->data;
  char *name;
  while (index < p_adv_report->dlen) {
    uint8_t field_length = p_data[index];
    uint8_t field_type = p_data[index + 1];

    if (field_type == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME) {
      name = (char *) &p_data[index + 2];
      name[field_length - 1] = '\0';

      if (strcmp(target_name, name) == 0) {
        return true;
      }
    }
    index += field_length + 1;
  }
  return false;
}

static bool is_device_connectable(const ble_gap_evt_adv_report_t *p_adv_report) {
  if (p_adv_report->type == BLE_GAP_ADV_TYPE_ADV_NONCONN_IND) {
    return false;
  }
  return true;
}

static bool is_device_in_pair_mode(const ble_gap_evt_adv_report_t *p_adv_report) {
  if (p_adv_report->data[2] == 0x06) {
    return true;
  }
  return false;
}

static uint32_t adv_data_parser(uint32_t pair_mode, const ble_gap_evt_adv_report_t *p_adv_data) {
  uint32_t ret_val = NRF_SUCCESS;

  if (is_name_present(PERIPHERAL_NAME, p_adv_data)) {
    if (is_device_in_pair_mode(p_adv_data) == (bool) pair_mode) {
      if (is_device_connectable(p_adv_data)) {
        ret_val = 1;
      } else {
        ret_val = 0;
      }
    }
  }
  return ret_val;
}

/**@brief Byte array type. */
typedef struct {
  uint16_t size;   /**< Number of array entries. */
  uint8_t *p_data; /**< Pointer to array entries. */
} uint8_array_t;

static uint32_t adv_report_parse_4s(uint8_t type, uint8_array_t *p_advdata, uint8_array_t *p_typedata) {
  uint32_t index = 0;
  uint8_t *p_data;

  p_data = p_advdata->p_data;

  while (index < p_advdata->size) {
    uint8_t field_length = p_data[index];
    uint8_t field_type = p_data[index + 1];

    if (field_type == type) {
      p_typedata->p_data = &p_data[index + 2];
      p_typedata->size = field_length - 1;
      return NRF_SUCCESS;
    }
    index += field_length + 1;
  }
  return NRF_ERROR_NOT_FOUND;
}

#define BR4S_SYSTEM_TYPE ((uint16_t) 0x8003)
#define BR4S_SYSTEM_STYPE ((uint16_t) 0x0000)

#define __packed __attribute__((packed))
struct br4s_ble_adv_dev_info_t {
  uint16_t com_id;
  uint8_t dev_mac[BLE_GAP_ADDR_LEN]; /*WTF? BLE Mac in advertising packets by
                                       default */
  uint16_t dev_type;                 /* Br3.0, CO2+, Ir-remote */
  uint16_t dev_stype;                /* sub-type */
  uint8_t dev_pair;                  /* ready for pairing */
} __attribute__((packed));

/*Private functions*/
static uint32_t adv_data_parser_4s(uint32_t pair_mode, const ble_gap_evt_adv_report_t *p_adv_data) {
  uint32_t ret_val = NRF_SUCCESS;

  br4s_ble_adv_dev_info_t *p_local_adv_data;
  uint8_array_t b_local_raw_adv_data = {0};
  uint8_array_t b_local_adv_data = {0};

  if ((p_adv_data->data[14] == 0x80) && (p_adv_data->data[13] == 0x03)) {
    b_local_raw_adv_data.p_data = (uint8_t *) p_adv_data->data;
    b_local_raw_adv_data.size = p_adv_data->dlen;

    if (NRF_SUCCESS ==
        adv_report_parse_4s(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, &b_local_raw_adv_data, &b_local_adv_data)) {
      p_local_adv_data = (br4s_ble_adv_dev_info_t *) b_local_adv_data.p_data;
      if (p_local_adv_data->dev_type == BR4S_SYSTEM_TYPE && p_local_adv_data->dev_stype == BR4S_SYSTEM_STYPE &&
          p_local_adv_data->dev_pair == (pair_mode ? 1 : 0)) {
        ret_val = true;
      } else {
        ret_val = false;
      }
    } else {
      ret_val = false;
    }
  }
  return ret_val;
}

#define BLE_SERVICE_NAME PERIPHERAL_NAME
void xxx() {
  uint8_t raw[3 + 2 + sizeof(BLE_SERVICE_NAME) - 1]{};
  // adv_report.dlen = 3 + 2 + sizeof(PERIPHERAL_NAME) - 1;
  // uint8_t *raw = adv_report.data;
  raw[0] = 2;
  raw[1] = 0x01;  // BLE_GAP_AD_TYPE_FLAGS
  raw[2] = 6;
  raw[3] = sizeof(BLE_SERVICE_NAME) - 1 + 1;
  raw[4] = 0x09;  // BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME
  memcpy(&raw[5], BLE_SERVICE_NAME, sizeof(BLE_SERVICE_NAME) - 1);
  // esp_ble_gap_config_adv_data_raw(raw, sizeof(raw));
  ESP_LOGD(TAG, "%s", cloak::hexencode(raw, sizeof(raw)).c_str());
}

#define ESP_BD_ADDR_LEN 6
#define ESP_BLE_AD_TYPE_FLAG 0x01
#define ESP_BLE_AD_TYPE_NAME_CMPL 0x09
#define ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE 0xFF
#define ESP_BLE_ADV_DATA_LEN_MAX 31

template<typename T, size_t V> struct BleAdvField {
  uint8_t size{sizeof(BleAdvField<T, V>) - 1};
  uint8_t type{V};
  T data;
  BleAdvField(const T data)
  // : size(sizeof(BleAdvField<T, V>) - 1), type(V)
  {
    if constexpr (std::is_array<T>::value) {
      memcpy(this->data, data, sizeof(T));
    } else {
      this->data = data;
    }
  }
} PACKED;

void xxx_4s() {
  struct TionManuData {
    uint8_t mac[ESP_BD_ADDR_LEN];
    dentra::tion::tion_dev_info_t::device_type_t type;
    struct {
      bool pair : 8;
    };
  } PACKED;
  struct TionAdvManuData {
    uint16_t vnd_id;
    TionManuData data;
  } PACKED manu_data{
      .vnd_id = 0xFFFF,
      {
          .mac = {0x49, 0x59, 0x13, 0x86, 0xa1, 0xf5},
          .type = dentra::tion::tion_dev_info_t::device_type_t::BR4S,
          .pair = true,
      },
  };

  ESP_LOGD(TAG, "manu_data: %s", cloak::hexencode(&manu_data, sizeof(manu_data)).c_str());
  ESP_LOGD(TAG, "name     : %s", cloak::hexencode(BLE_SERVICE_NAME, sizeof(BLE_SERVICE_NAME) - 1).c_str());

  static_assert(sizeof(TionAdvManuData) == 13);

// #define BLE_SERVICE_NAME "Br Lite"
#define BLE_SERVICE_NAME "Breezer 4S"

  // 02 01 06 0e ff ff ff 49 59 13 86 a1 f5 03 80 00 00 01 0b 09 42 72 65 65 7a 65 72 20 34 53
  //                    02.01.06.0E.FF.FF.00.00.00.00.00.00.00.03.80.00.00.01.04.09.42.72.65
  ESP_LOGD(TAG, "adv -: 02 01 06 0E FF FF FF 49 59 13 86 A1 F5 03 80 00 00 01 0B 09 42 72 65 65 7A 65 72 20 34 53");

  uint8_t raw[(2 + 1) + (2 + sizeof(BLE_SERVICE_NAME) - 1) + (2 + sizeof(manu_data))]{};
  static_assert(sizeof(raw) <= ESP_BLE_ADV_DATA_LEN_MAX);

  raw[0] = 2;                     // len
  raw[1] = ESP_BLE_AD_TYPE_FLAG;  // BLE_GAP_AD_TYPE_FLAGS
  raw[2] = 6;                     // flags

  raw[3] = sizeof(manu_data) + 1;
  raw[4] = ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE;
  memcpy(&raw[5], &manu_data, sizeof(manu_data));

  raw[5 + sizeof(manu_data) + 0] = sizeof(BLE_SERVICE_NAME);   // len
  raw[5 + sizeof(manu_data) + 1] = ESP_BLE_AD_TYPE_NAME_CMPL;  // BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME
  memcpy(&raw[5 + sizeof(manu_data) + 2], BLE_SERVICE_NAME, sizeof(BLE_SERVICE_NAME) - 1);
  ESP_LOGD(TAG, "adv S: %s", cloak::hexencode(raw, sizeof(raw)).c_str());

  struct {
    uint8_t flag_size;
    uint8_t flag_type;
    uint8_t flag_data;
    uint8_t manu_size;
    uint8_t manu_type;
    TionAdvManuData manu_data;
    uint8_t name_size;
    uint8_t name_type;
    uint8_t name_data[sizeof(BLE_SERVICE_NAME) - 1];
  } PACKED adv_data{
      .flag_size = 2,
      .flag_type = ESP_BLE_AD_TYPE_FLAG,
      .flag_data = 6,
      .manu_size = sizeof(manu_data) + 1,
      .manu_type = ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE,
      .manu_data = manu_data,
      .name_size = sizeof(BLE_SERVICE_NAME),
      .name_type = ESP_BLE_AD_TYPE_NAME_CMPL,
      .name_data = {},
  };
  memcpy(adv_data.name_data, BLE_SERVICE_NAME, sizeof(BLE_SERVICE_NAME) - 1);
  static_assert(sizeof(adv_data) == 30);
  ESP_LOGD(TAG, "adv X: %s", cloak::hexencode(&adv_data, sizeof(adv_data)).c_str());

  struct {
    BleAdvField<uint8_t, ESP_BLE_AD_TYPE_FLAG> flags;
    BleAdvField<TionAdvManuData, ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE> manu;
    BleAdvField<char[sizeof(BLE_SERVICE_NAME) - 1], ESP_BLE_AD_TYPE_NAME_CMPL> name;
  } PACKED adv_data_z{.flags{6},
                      .manu{{
                          .vnd_id = 0xFFFF,
                          .data{
                              .mac = {0x49, 0x59, 0x13, 0x86, 0xa1, 0xf5},
                              .type = dentra::tion::tion_dev_info_t::device_type_t::BR4S,
                              .pair = true,
                          },
                      }},
                      .name{BLE_SERVICE_NAME}};
  static_assert(sizeof(adv_data_z.flags) == 3);
  static_assert(sizeof(adv_data_z.manu) == sizeof(TionAdvManuData) + 2);
  static_assert(sizeof(adv_data_z.name) == sizeof(BLE_SERVICE_NAME) + 1);
  static_assert(sizeof(adv_data_z) == 30);
  ESP_LOGD(TAG, "adv Z: %s", cloak::hexencode(&adv_data_z, sizeof(adv_data_z)).c_str());

  ble_gap_evt_adv_report_t adv_report{};
  adv_report.dlen = sizeof(raw);
  memcpy(adv_report.data, raw, sizeof(raw));
  ble_gap_evt_adv_report_t *p_adv_report = &adv_report;
  uint32_t res = adv_data_parser_4s(1, p_adv_report);
  ESP_LOGD(TAG, "%s in pair mode %s\n", BLE_SERVICE_NAME, ONOFF(res == NRF_SUCCESS));
}

bool test_rc() {
  struct {
    BleAdvField<uint8_t, ESP_BLE_AD_TYPE_FLAG> flags;
    BleAdvField<char[sizeof(PERIPHERAL_NAME) - 1], ESP_BLE_AD_TYPE_NAME_CMPL> name;
  } PACKED new_adv_data{.flags{6}, .name{PERIPHERAL_NAME}};

  ble_gap_evt_adv_report_t adv_report{.type = BLE_GAP_ADV_TYPE_ADV_IND, .dlen = sizeof(new_adv_data)};
  memcpy(adv_report.data, &new_adv_data, sizeof(new_adv_data));

  ESP_LOGD(TAG, "%s", cloak::hexencode(&adv_report, sizeof(adv_report)).c_str());

  bool res = adv_data_parser(1, &adv_report);
  ESP_LOGD(TAG, "%s in pair mode %s\n", PERIPHERAL_NAME, ONOFF(res));
  return res;
}

// [14:37:12][W][tion_rc:105]: Packet: 80.10.00.3A.3B.32.32.43.A9.B0.9E.9A.88.52.10.BB.AA (17)
// [14:37:12][W][tion-api-ble-lt:107]: Invalid frame crc: 9970
// [14:37:12][W][tion_rc:105]: Packet: 80.10.00.3A.17.32.41.5E.33.64.B0.E4.21.FB.68.BB.AA (17)
// [14:37:12][W][tion-api-ble-lt:107]: Invalid frame crc: 2C87
// [14:37:12][W][tion_rc:105]: Packet: 80.10.00.3A.3D.32.33.ED.9A.69.68.18.18.30.72.BB.AA (17)
// [14:37:12][W][tion-api-ble-lt:107]: Invalid frame crc: 2AD3

bool test_rc_4s() {
  ESP_LOGD(TAG, "%d : %d", 1, -1);
  ESP_LOGD(TAG, "%02x : %02x", 1, -1 & 0xFF);
  ESP_LOGD(TAG, "%02x : %02x", 2, -2 & 0xFF);
  ESP_LOGD(TAG, "%02x : %02x", 2, -128 & 0xFF);
  for (int8_t i = 0; i < 6; i++) {
    ESP_LOGD(TAG, "%d : %d", i, 6 - 1 - i);
  }

  xxx_4s();
  return test_rc();

  bool res = true;
  using TionRCBleProtocol = dentra::tion::TionLtBleProtocol;

  TionRCBleProtocol pr_(false);
  pr_.reader = [](const TionRCBleProtocol::frame_spec_type &data, size_t size) {
    ESP_LOGD(TAG, "RX %s", cloak::hexencode(&data, size).c_str());
    ESP_LOGD(TAG, "  type: %04X: %s", data.type, cloak::hexencode(data.data, size - data.head_size()).c_str());
  };
  pr_.writer = [](const uint8_t *data, size_t size) {
    ESP_LOGD(TAG, "tx %s", cloak::hexencode(data, size).c_str());
    return true;
  };
  auto do_read = [&pr_](const std::vector<uint8_t> &arr) { pr_.read_data(arr.data(), arr.size()); };

  do_read(cloak::from_hex("80.10.00.3A.3B.32.32.43.A9.B0.9E.9A.88.52.10.BB.AA"));

  uint32_t req_id = 0x1052889A;
  pr_.write_frame(dentra::tion_4s::FRAME_TYPE_STATE_REQ, &req_id, sizeof(req_id));

  return res;
}

REGISTER_TEST(test_rc);
REGISTER_TEST(test_rc_4s);
