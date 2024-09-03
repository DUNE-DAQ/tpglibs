/**
 * @file AbstractFactory.hxx
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TPGLIBS_ABSTRACTFACTORY_HXX_
#define TPGLIBS_ABSTRACTFACTORY_HXX_

namespace tpglibs {

template <typename T>
std::shared_ptr<AbstractFactory<T>> AbstractFactory<T>::s_single_factory = nullptr;

template <typename T>
typename AbstractFactory<T>::name_creator_map& AbstractFactory<T>::get_creators() {
  static name_creator_map s_creators;
  return s_creators;
}

template <typename T>
void AbstractFactory<T>::register_creator(const std::string& processor_name, create_processor_func creator) {
  name_creator_map& creators = get_creators();
  auto it = creators.find(processor_name);

  if (it == creators.end()) {
    creators[processor_name] = creator;
    return;
  }
  throw std::runtime_error("Attempted to overwrite a creator in factory with " + processor_name);
  return;
}

template <typename T>
std::shared_ptr<T> AbstractFactory<T>::create_processor(const std::string& processor_name) {
  name_creator_map& creators = get_creators();
  auto it = creators.find(processor_name);

  if (it != creators.end()) {
    return it->second();
  }

  throw std::runtime_error("Factory failed to find " + processor_name);
  return nullptr;
}

template <typename T>
std::shared_ptr<AbstractFactory<T>> AbstractFactory<T>::get_instance()
{
  if (s_single_factory == nullptr) {
    s_single_factory = std::make_shared<AbstractFactory<T>>();
  }

  return s_single_factory;
}

} // namespace tpglibs

#endif // TPGLIBS_ABSTRACTFACTORY_HXX_
