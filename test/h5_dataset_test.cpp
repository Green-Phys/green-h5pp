#include <catch2/catch_test_macros.hpp>

#include <array>
#include <complex>
#include <filesystem>

#include "green/h5pp/archive.h"

using namespace std::literals;

template <typename T, size_t N>
struct NDArray {
  NDArray(std::array<size_t, N> new_shape, T val) :
      _shape(new_shape),
      _data(std::accumulate(new_shape.begin(), new_shape.end(), 1ul, std::multiplies<size_t>()), val) {}

  size_t                       size() const { return _data.size(); }
  size_t                       dim() const { return N; }

  T*                           data() const { return _data.data(); }
  T*                           data() { return _data.data(); }

  const std::array<size_t, N>& shape() const { return _shape; }

  void                         reshape(const std::vector<size_t>& new_shape) {
    assert(new_shape.size() == _shape.size());
    std::copy(new_shape.begin(), new_shape.end(), _shape.begin());
    size_t new_size = std::accumulate(new_shape.begin(), new_shape.end(), 1ul, std::multiplies<size_t>());
    _data.resize(new_size);
  }

  std::array<size_t, N> _shape;
  std::vector<T>        _data;
};

TEST_CASE("Dataset Operations") {
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
    REQUIRE_THROWS_AS(dataset >> data, green::h5pp::hdf5_not_a_dataset_error);
  }

  SECTION("Type Conversion") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    double               data_d;
    float                data_s;
    int                  data_i;
    long                 data_l;
    std::complex<double> data_z;
    ar["GROUP/SCALAR_DATASET"] >> data_d >> data_s >> data_i >> data_l;
    REQUIRE(std::abs(data_d - 1.0) < 1e-10);
    REQUIRE(std::abs(data_s - 1.0) < 1e-6);
    REQUIRE(data_i == 1);
    REQUIRE(data_l == 1);
    REQUIRE_THROWS_AS(ar["GROUP/SCALAR_DATASET"] >> data_z, green::h5pp::hdf5_data_conversion_error);
  }

  SECTION("Read Scalar") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    double               data;
    ar["GROUP/SCALAR_DATASET"] >> data;
    REQUIRE(std::abs(data - 1.0) < 1e-12);
    REQUIRE_THROWS_AS(ar["GROUP/VECTOR_DATASET"] >> data, green::h5pp::hdf5_not_a_scalar_error);
  }

  SECTION("Write Scalar") {
    std::string          filename = TEST_PATH + "/test_write.h5"s;
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
    std::string          filename = TEST_PATH + "/test_write.h5"s;
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
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write Complex Scalar") {
    std::string          filename = TEST_PATH + "/test_write.h5"s;
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

  SECTION("Read Array") {
    std::string            filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive   ar(filename, "r");
    auto                   group = ar["GROUP"];
    std::array<double, 10> data{{10}};
    group["VECTOR_DATASET"] >> data;
    REQUIRE(std::abs(data[0] - 0.652442) < 1e-6);
  }

  SECTION("Read Resizable Array") {
    std::string          filename = TEST_PATH + "/test.h5"s;
    green::h5pp::archive ar(filename, "r");
    auto                 group = ar["GROUP"];
    std::vector<double>  data;
    group["VECTOR_DATASET"] >> data;
    REQUIRE(data.size() == 10);
    REQUIRE(std::abs(data[0] - 0.652442) < 1e-6);
  }

  SECTION("Write Array") {
    std::string            filename = TEST_PATH + "/test_write.h5"s;
    green::h5pp::archive   ar(filename, "w");
    auto                   group = ar["GROUP"];
    std::array<double, 10> data;
    data.fill(10);
    group["DATASET"] << data;
    std::vector<double> new_data;
    group["DATASET"] >> new_data;
    REQUIRE(data.size() == new_data.size());
    REQUIRE(std::equal(data.begin(), data.end(), new_data.begin(),
                       [](double a, double b) { return std::abs(a - b) < 1e-10; }));
    std::filesystem::remove(std::filesystem::path(filename));
  }

  SECTION("Write Complex Array") {
    std::string                          filename = TEST_PATH + "/test_write.h5"s;
    green::h5pp::archive                 ar(filename, "w");
    auto                                 group = ar["GROUP"];
    std::array<std::complex<double>, 10> data;
    data.fill(std::complex<double>(5, 10));
    group["DATASET"] << data;
    std::vector<std::complex<float>> new_data;
    group["DATASET"] >> new_data;
    REQUIRE(data.size() == new_data.size());
    REQUIRE(std::equal(data.begin(), data.end(), new_data.begin(),
                       [](const std::complex<double>& a, const std::complex<double>& b) {
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
    REQUIRE(std::abs(*data.data() - 0.335167) < 1e-6);
  }

  SECTION("Write NDArray") {
    std::string          filename = TEST_PATH + "/test_write.h5"s;
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
}
