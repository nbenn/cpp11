#pragma once

#include <initializer_list>  // for initializer_list
#include <stdexcept>         // for out_of_range

#include "cpp11/R.hpp"          // for SEXP, SEXPREC, protect_sexp
#include "cpp11/named_arg.hpp"  // for named_arg
#include "cpp11/protect.hpp"    // for protect, protect::function, safe
#include "cpp11/sexp.hpp"       // for sexp
#include "cpp11/r_vector.hpp"     // for vector, vector<>::proxy, vector<>::...

// Specializations for list

namespace cpp11 {

template <>
inline SEXP r_vector<SEXP>::valid_type(SEXP data) {
  if (TYPEOF(data) != VECSXP) {
    throw type_error(VECSXP, TYPEOF(data));
  }
  return data;
}

template <>
inline SEXP r_vector<SEXP>::operator[](const R_xlen_t pos) const {
  return VECTOR_ELT(data_, pos);
}

template <>
inline SEXP r_vector<SEXP>::operator[](const r_string& name) const {
  SEXP names = this->names();
  R_xlen_t size = Rf_xlength(names);

  for (R_xlen_t pos = 0; pos < size; ++pos) {
    auto cur = Rf_translateCharUTF8(STRING_ELT(names, pos));
    if (name == cur) {
      return operator[](pos);
    }
  }
  return R_NilValue;
}

template <>
inline SEXP r_vector<SEXP>::at(const R_xlen_t pos) const {
  if (pos < 0 || pos >= length_) {
    throw std::out_of_range("doubles");
  }
  return VECTOR_ELT(data_, pos);
}

template <>
inline SEXP* r_vector<SEXP>::get_p(bool, SEXP) {
  return nullptr;
}

template <>
inline void r_vector<SEXP>::const_iterator::fill_buf(R_xlen_t) {
  return;
}

typedef r_vector<SEXP> list;

namespace writable {

template <>
inline typename r_vector<SEXP>::proxy& r_vector<SEXP>::proxy::operator=(SEXP rhs) {
  SET_VECTOR_ELT(data_, index_, rhs);
  return *this;
}

template <>
inline r_vector<SEXP>::proxy::operator SEXP() const {
  return VECTOR_ELT(data_, index_);
}

template <>
inline r_vector<SEXP>::r_vector(std::initializer_list<SEXP> il)
    : cpp11::r_vector<SEXP>(safe[Rf_allocVector](VECSXP, il.size())), capacity_(il.size()) {
  protect_ = protect_sexp(data_);
  auto it = il.begin();
  for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
    SET_VECTOR_ELT(data_, i, *it);
  }
}

template <>
inline r_vector<SEXP>::r_vector(std::initializer_list<named_arg> il)
    : cpp11::r_vector<SEXP>(safe[Rf_allocVector](VECSXP, il.size())), capacity_(il.size()) {
  try {
    unwind_protect([&] {
      protect_ = protect_sexp(data_);
      Rf_setAttrib(data_, R_NamesSymbol, Rf_allocVector(STRSXP, capacity_));
      sexp nms(names());
      auto it = il.begin();
      for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
        SET_VECTOR_ELT(data_, i, it->value());
        SET_STRING_ELT(nms, i, Rf_mkCharCE(it->name(), CE_UTF8));
      }
    });
  } catch (const unwind_exception& e) {
    release_protect(protect_);
    throw e;
  }
}

template <>
inline void r_vector<SEXP>::reserve(R_xlen_t new_capacity) {
  data_ = data_ == R_NilValue ? safe[Rf_allocVector](VECSXP, new_capacity)
                              : safe[Rf_xlengthgets](data_, new_capacity);

  SEXP old_protect = protect_;
  protect_ = protect_sexp(data_);
  release_protect(old_protect);

  capacity_ = new_capacity;
}

template <>
inline void r_vector<SEXP>::push_back(SEXP value) {
  while (length_ >= capacity_) {
    reserve(capacity_ == 0 ? 1 : capacity_ *= 2);
  }
  SET_VECTOR_ELT(data_, length_, value);
  ++length_;
}

typedef r_vector<SEXP> list;

}  // namespace writable

}  // namespace cpp11