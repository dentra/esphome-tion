#pragma once

#include <string>
#include <cstdint>

namespace esphome {

enum EntityCategory : uint8_t {
  ENTITY_CATEGORY_NONE = 0,
  ENTITY_CATEGORY_CONFIG = 1,
  ENTITY_CATEGORY_DIAGNOSTIC = 2,
};

// The generic Entity base class that provides an interface common to all Entities.
class EntityBase {
 public:
  EntityBase() : EntityBase("") {}
  explicit EntityBase(std::string name) : name_(name) {}

  // Get/set the name of this Entity
  const std::string &get_name() const { return this->name_; }
  void set_name(const std::string &name) { this->name_ = name; }

  // Get the sanitized name of this Entity as an ID. Caching it internally.
  const std::string &get_object_id() { return this->object_id_; }

  // Get the unique Object ID of this Entity
  uint32_t get_object_id_hash() { return this->object_id_hash_; }

  // Get/set whether this Entity should be hidden from outside of ESPHome
  bool is_internal() const { return this->internal_; }
  void set_internal(bool internal) { this->internal_ = internal; };

  // Check if this object is declared to be disabled by default.
  // That means that when the device gets added to Home Assistant (or other clients) it should
  // not be added to the default view by default, and a user action is necessary to manually add it.
  bool is_disabled_by_default() const { return this->disabled_by_default_; }
  void set_disabled_by_default(bool disabled_by_default) { this->disabled_by_default_ = disabled_by_default; }

  // Get/set the entity category.
  EntityCategory get_entity_category() const { return this->entity_category_; }
  void set_entity_category(EntityCategory entity_category) { this->entity_category_ = entity_category; }

  // Get/set this entity's icon
  const std::string &get_icon() const { return this->icon_; }
  void set_icon(const std::string &name) { this->icon_ = name; }

 protected:
  /// The hash_base() function has been deprecated. It is kept in this
  /// class for now, to prevent external components from not compiling.
  virtual uint32_t hash_base() { return 0L; }
  void calc_object_id_();

  std::string name_;
  std::string object_id_;
  std::string icon_;
  uint32_t object_id_hash_;
  bool internal_{false};
  bool disabled_by_default_{false};
  EntityCategory entity_category_{ENTITY_CATEGORY_NONE};
};

}  // namespace esphome
