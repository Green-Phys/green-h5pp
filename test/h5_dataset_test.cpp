/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <complex>
#include <filesystem>

#include "green/h5pp/archive.h"
#include "test_common.h"

using namespace std::literals;

template <typename T, size_t N>
struct NDArray {
  NDArray() {}
  NDArray(std::array<size_t, N> new_shape, T val) :
      _shape(new_shape), _data(std::accumulate(new_shape.begin(), new_shape.end(), 1ul, std::multiplies<size_t>()), val) {}

  size_t                       size() const { return _data.size(); }
  size_t                       dim() const { return N; }

  const T*                     data() const { return _data.data(); }
  T*                           data() { return _data.data(); }

  const std::array<size_t, N>& shape() const { return _shape; }

  template <typename C>
  void resize(const C& new_shape) {
    assert(new_shape.size() == _shape.size());
    std::copy(new_shape.begin(), new_shape.end(), _shape.begin());
    size_t new_size = std::accumulate(new_shape.begin(), new_shape.end(), 1ul, std::multiplies<size_t>());
    _data.resize(new_size);
  }

  std::array<size_t, N> _shape;
  std::vector<T>        _data;
};

TEST_CASE("Dataset Operations") {
  SECTION("Assign uninitialized objects") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    green::h5pp::object  group;
    green::h5pp::object  dataset;
    double               data;
    REQUIRE(group.type() == green::h5pp::INVALID);
    REQUIRE_THROWS(dataset >> data);
    {
      auto new_group   = ar["GROUP"];
      auto new_dataset = ar["GROUP/SCALAR_DATASET"];
      group            = new_group;
      dataset          = new_dataset;
    }
    dataset >> data;
    REQUIRE(std::abs(data - 1.0) < 1e-10);
  }
  SECTION("Read Usupported Type") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    auto                 dataset = ar["GROUP/SCALAR_DATASET"];
    std::stringstream    data;
    REQUIRE_THROWS_AS(dataset >> data, green::h5pp::hdf5_unsupported_type_error);
  }

  SECTION("Read Not Dataset") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    auto                 dataset = ar["GROUP"];
    double               data;
    double*              data_ptr = &data;
    REQUIRE_THROWS_AS(dataset >> data, green::h5pp::hdf5_not_a_dataset_error);
    REQUIRE_THROWS_AS(dataset >> data_ptr, green::h5pp::hdf5_not_a_dataset_error);
  }

  SECTION("Type Conversion") {
    std::string           filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive  ar(filename, "r");
    double                data_d;
    float                 data_s;
    int                   data_i;
    long                  data_l;
    std::complex<double>  data_z;
    std::complex<double>* data_z_ptr = &data_z;
    ar["GROUP/SCALAR_DATASET"] >> data_d >> data_s >> data_i >> data_l;
    REQUIRE(std::abs(data_d - 1.0) < 1e-10);
    REQUIRE(std::abs(data_s - 1.0) < 1e-6);
    REQUIRE(data_i == 1);
    REQUIRE(data_l == 1);
    REQUIRE_THROWS_AS(ar["GROUP/SCALAR_DATASET"] >> data_z, green::h5pp::hdf5_data_conversion_error);
    REQUIRE_THROWS_AS(ar["GROUP/SCALAR_DATASET"] >> data_z_ptr, green::h5pp::hdf5_data_conversion_error);
    std::vector<double>               data_dv;
    std::vector<std::complex<double>> data_zv;
    REQUIRE_NOTHROW(ar["GROUP/VECTOR_DATASET"] >> data_dv);
    REQUIRE_THROWS_AS(ar["GROUP/VECTOR_DATASET"] >> data_zv, green::h5pp::hdf5_data_conversion_error);
  }

  SECTION("Read Scalar") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    double               data;
    ar["GROUP/SCALAR_DATASET"] >> data;
    REQUIRE(std::abs(data - 1.0) < 1e-12);
    REQUIRE_THROWS_AS(ar["GROUP/VECTOR_DATASET"] >> data, green::h5pp::hdf5_not_a_scalar_error);
    ar["GROUP/NDARRAY_SCALAR"] >> data;
    REQUIRE(std::abs(data - 1.0) < 1e-12);
  }

  SECTION("Write Scalar") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP"];
    double               data  = 10;
    group["DATASET"] << data;
    double new_data = -1;
    ar["GROUP/DATASET"] >> new_data;
    REQUIRE(std::abs(data - new_data) < 1e-10);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Update Scalar") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP"];
    double               data  = 10;
    group["DATASET"] << data;
    double new_data = -1;
    ar["GROUP/DATASET"] >> new_data;
    REQUIRE(std::abs(data - new_data) < 1e-10);
    group["DATASET"] << 15;
    ar["GROUP/DATASET"] >> new_data;
    REQUIRE(std::abs(new_data - 15) < 1e-10);
    NDArray<double, 3> nd_data(
        std::array<size_t, 3>{
            {1, 1, 1}
    },
        5.0);
    group["NDARRAY_SCALAR"] << nd_data;
    group["NDARRAY_SCALAR"] >> new_data;
    REQUIRE(std::abs(new_data - 5) < 1e-10);
    group["NDARRAY_SCALAR"] << data;
    group["NDARRAY_SCALAR"] >> new_data;
    REQUIRE(std::abs(new_data - data) < 1e-10);
    nd_data.resize(std::array<size_t, 3>{
        {1, 1, 2}
    });
    group["NDARRAY"] << nd_data;
    REQUIRE_THROWS_AS(group["NDARRAY"] << data, green::h5pp::hdf5_not_a_scalar_error);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write Complex Scalar") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP"];
    std::complex<double> data(5.0, 10.0);
    group["DATASET"] << data;
    std::complex<double> new_data(-1.0, -1.0);
    ar["GROUP/DATASET"] >> new_data;
    REQUIRE(std::abs(data.real() - new_data.real()) < 1e-10);
    REQUIRE(std::abs(data.imag() - new_data.imag()) < 1e-10);
    double new_data_d;
    REQUIRE_THROWS_AS(ar["GROUP/DATASET"] >> new_data_d, green::h5pp::hdf5_data_conversion_error);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Read String") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    auto                 group = ar["GROUP"];
    std::string          data;
    group["STRING_DATASET"] >> data;
    REQUIRE(data == "HELLO WORLD!"s);
    REQUIRE_THROWS_AS(group["SCALAR_DATASET"] >> data, green::h5pp::hdf5_read_error);
  }

  SECTION("Write into readonly") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    double               a = 10;
    REQUIRE_THROWS_AS(ar["GROUP/SCALAR_DATASET"] << a, green::h5pp::hdf5_write_error);
  }

  SECTION("Write String") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP"];
    std::string          data  = "HELLO WORLD!";
    group["STRING_DATASET"] << data;
    std::string new_data = "";
    ar["GROUP/STRING_DATASET"] >> new_data;
    REQUIRE(data == new_data);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Read Array") {
    std::string            filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive   ar(filename, "r");
    auto                   group = ar["GROUP"];
    std::array<double, 10> data{{10}};
    group["VECTOR_DATASET"] >> data;
    REQUIRE(std::abs(data[0] - 0.157635) < 1e-6);
    std::array<double, 30> array_30;
    REQUIRE_THROWS_AS(group["NDARRAY_DATASET"] >> array_30, green::h5pp::hdf5_read_error);
  }

  SECTION("Read Resizable Array") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    auto                 group = ar["GROUP"];
    std::vector<double>  data;
    group["VECTOR_DATASET"] >> data;
    REQUIRE(data.size() == 10);
    REQUIRE(std::abs(data[0] - 0.157635) < 1e-6);
    std::array<double, 60> array_60;
    group["NDARRAY_DATASET"] >> data;
    group["NDARRAY_DATASET"] >> array_60;
    REQUIRE(std::equal(data.begin(), data.end(), array_60.begin(), [](double a, double b) { return std::abs(a - b) < 1e-10; }));
  }

  SECTION("Write Array") {
    std::string            filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive   ar(filename, "w");
    auto                   group = ar["GROUP"];
    std::array<double, 10> data;
    data.fill(10);
    group["DATASET"] << data;
    std::vector<double> new_data;
    group["DATASET"] >> new_data;
    REQUIRE(data.size() == new_data.size());
    REQUIRE(std::equal(data.begin(), data.end(), new_data.begin(), [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    REQUIRE_THROWS(group << data);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write Unsupported Type") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    std::stringstream    ss;
    REQUIRE_THROWS_AS(ar["DATASET"] << ss, green::h5pp::hdf5_unsupported_type_error);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Update Array") {
    std::string            filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive   ar(filename, "w");
    auto                   group = ar["GROUP"];
    std::array<double, 10> data;
    data.fill(10);
    group["DATASET"] << data;
    std::vector<double> new_data;
    group["DATASET"] >> new_data;
    data.fill(15);
    group["DATASET"] << data;
    group["DATASET"] >> new_data;
    std::array<double, 12> bigger_data;
    REQUIRE(data.size() == new_data.size());
    REQUIRE(std::equal(data.begin(), data.end(), new_data.begin(), [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    REQUIRE_THROWS_AS(group["DATASET"] << bigger_data ,green::h5pp::hdf5_write_error);
    NDArray<double, 2> nd_data(
        std::array<size_t, 2>{
            {5, 5}
    },
        5.0);
    group["ND_DATASET"] << nd_data;
    std::array<double, 25> data_25;
    data_25.fill(25);
    group["ND_DATASET"] << data_25;
    group["ND_DATASET"] >> nd_data;
    REQUIRE(
        std::equal(data_25.begin(), data_25.end(), nd_data.data(), [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    NDArray<double, 2> bigger_nd_data(
        std::array<size_t, 2>{
            {15, 15}
    },
        5.0);
    REQUIRE_THROWS_AS(group["ND_DATASET"] << bigger_nd_data ,green::h5pp::hdf5_write_error);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Update NDArray") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    NDArray<double, 3>   nd_data(
        std::array<size_t, 3>{
            {1, 1, 1}
    },
        5.0);
    ar["DATASET"] << nd_data;
    ar.close();
    ar.open(filename, "a");
    NDArray<double, 3> new_data;
    ar["DATASET"] >> new_data;
    REQUIRE(std::equal(nd_data.data(), nd_data.data() + nd_data.size(), new_data.data(),
                       [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    std::fill(new_data._data.begin(), new_data._data.end(), 15);
    ar["DATASET"] << new_data;
    REQUIRE_FALSE(std::equal(nd_data.data(), nd_data.data() + nd_data.size(), new_data.data(),
                             [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    ar["DATASET"] >> nd_data;
    REQUIRE(std::equal(nd_data.data(), nd_data.data() + nd_data.size(), new_data.data(),
                       [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    ar.close();
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write Complex Array") {
    std::string                          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive                 ar(filename, "w");
    auto                                 group = ar["GROUP"];
    std::array<std::complex<double>, 10> data;
    data.fill(std::complex<double>(5, 10));
    group["DATASET"] << data;
    std::vector<std::complex<float>> new_data;
    group["DATASET"] >> new_data;
    REQUIRE(data.size() == new_data.size());
    REQUIRE(
        std::equal(data.begin(), data.end(), new_data.begin(), [](const std::complex<double>& a, const std::complex<double>& b) {
          return (std::abs(a.real() - b.real()) + std::abs(a.imag() - b.imag())) < 1e-10;
        }));
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Read NDArray") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    auto                 group = ar["GROUP"];
    NDArray<double, 2>   data(
        std::array<size_t, 2>{
            {1, 1}
    },
        5.0);
    group["NDARRAY_DATASET"] >> data;
    REQUIRE(std::abs(*data.data() - 0.110326) < 1e-6);
  }

  SECTION("Write NDArray") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP"];
    NDArray<double, 3>   data(
        std::array<size_t, 3>{
            {10, 5, 5}
    },
        5.0);
    group["DATASET"] << data;
    NDArray<double, 3> data_new(
        std::array<size_t, 3>{
            {10, 5, 5}
    },
        0.0);
    group["DATASET"] >> data_new;
    REQUIRE(std::equal(data.data(), data.data() + data.size(), data_new.data(),
                       [](double a, double b) { return (std::abs(a - b)) < 1e-10; }));
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Read into Pointer") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    auto                 group = ar["GROUP"];
    NDArray<double, 2>   data(
        std::array<size_t, 2>{
            {10, 6}
    },
        5.0);
    group["NDARRAY_DATASET"] >> data.data();
    REQUIRE(std::abs(*data.data() - 0.110326) < 1e-6);
  }

  SECTION("Write different datatypes") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    bool                 b  = true;
    int32_t              i  = 10;
    uint32_t             ui = 10u;
    int64_t              l  = 20l;
    uint64_t             ul = 30ul;
    float                f  = 0.5f;
    double               d  = 1.5;
    std::complex<float>  cf(0.5, 1.2);
    std::complex<double> cd(1.5, 0.2);
    std::string          s = "ABCD";
    REQUIRE_NOTHROW(ar["b"] << b);
    REQUIRE_NOTHROW(ar["i"] << i);
    REQUIRE_NOTHROW(ar["ui"] << ui);
    REQUIRE_NOTHROW(ar["l"] << l);
    REQUIRE_NOTHROW(ar["ul"] << ul);
    REQUIRE_NOTHROW(ar["f"] << f);
    REQUIRE_NOTHROW(ar["d"] << d);
    REQUIRE_NOTHROW(ar["cf"] << cf);
    REQUIRE_NOTHROW(ar["cd"] << cd);
    REQUIRE_NOTHROW(ar["s"] << s);
    ar.close();
    b = i = ui = l = ul = f = d = 0;
    ar.open(filename, "r");
    REQUIRE_NOTHROW(ar["b"] >> b);
    REQUIRE_NOTHROW(ar["i"] >> i);
    REQUIRE_NOTHROW(ar["ui"] >> ui);
    REQUIRE_NOTHROW(ar["l"] >> l);
    REQUIRE_NOTHROW(ar["ul"] >> ul);
    REQUIRE_NOTHROW(ar["f"] >> f);
    REQUIRE_NOTHROW(ar["d"] >> d);
    REQUIRE_NOTHROW(ar["cf"] >> cf);
    REQUIRE_NOTHROW(ar["cd"] >> cd);
    REQUIRE_NOTHROW(ar["s"] >> s);
    REQUIRE(b);
    REQUIRE(i == 10);
    REQUIRE(ui == 10u);
    REQUIRE(l == 20l);
    REQUIRE(ul == 30ul);
    REQUIRE(std::abs(f - 0.5f) < 1e-12);
    REQUIRE(std::abs(d - 1.5) < 1e-12);
    REQUIRE(std::abs(cf - std::complex<float>(0.5, 1.2)) < 1e-12);
    REQUIRE(std::abs(cd - std::complex<double>(1.5, 0.2)) < 1e-12);
    REQUIRE(s == "ABCD");
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write String Vector") {
    std::string              filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive     ar(filename, "w");
    std::vector<std::string> sv{"ABC", "XYZZZZ"};
    REQUIRE_NOTHROW(ar["DATASET"] << sv);
    std::vector<std::string> out_sv;
    ar["DATASET"] >> out_sv;
    REQUIRE(sv == out_sv);
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Update Strings") {
    std::string              filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive     ar(filename, "w");
    std::string s = "ABC";
    std::vector<std::string> sv{"ABC", "XYZZZZ"};
    std::string s2 = "XYZ!@#";
    std::vector<std::string> sv2{"ABCDEF", "XYZZZZ123"};
    REQUIRE_NOTHROW(ar["DATASET_V"] << sv);
    REQUIRE_NOTHROW(ar["DATASET"] << s);
    ar.close();
    ar.open(filename, "a");
    REQUIRE_NOTHROW(ar["DATASET_V"] << sv2);
    REQUIRE_NOTHROW(ar["DATASET"] << s2);
    std::vector<std::string> out_sv;
    std::string out_s;
    ar["DATASET"] >> out_s;
    ar["DATASET_V"] >> out_sv;
    REQUIRE(sv2 == out_sv);
    REQUIRE(s2 == out_s);
    std::filesystem::remove(std::filesystem::path(filename));
  }
}
