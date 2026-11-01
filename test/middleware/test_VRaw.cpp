
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <array>
#include <cstring>
#include <cstdint>
#include "VRaw.h"

using namespace vraw;

TEST_CASE("Writer: primitive types", "[writer]")
{
    std::array<uint8_t, 64> buffer{};
    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT8(0x12) == Err::OK);
    REQUIRE(writer.add_INT8(-10) == Err::OK);
    REQUIRE(writer.add_UINT16(0x1234) == Err::OK);
    REQUIRE(writer.add_INT16(-1234) == Err::OK);
    REQUIRE(writer.add_UINT32(0x12345678) == Err::OK);
    REQUIRE(writer.add_INT32(-123456) == Err::OK);
    REQUIRE(writer.add_UINT64(0x123456789ABCDEF0ULL) == Err::OK);
    REQUIRE(writer.add_FLOAT(3.14f) == Err::OK);
    REQUIRE(writer.add_DOUBLE(3.14159265358979) == Err::OK);

    REQUIRE(writer.size() == sizeof(uint8_t) + sizeof(int8_t) +
                             sizeof(uint16_t) + sizeof(int16_t) +
                             sizeof(uint32_t) + sizeof(int32_t) +
                             sizeof(uint64_t) + sizeof(float) +
                             sizeof(double));
}

TEST_CASE("Writer: arrays and binary data", "[writer]")
{
    std::array<uint8_t, 32> buffer{};
    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    uint16_t values[] = {100, 200, 300, 400};
    uint8_t binary[] = {0xAA, 0xBB, 0xCC, 0xDD};

    REQUIRE(writer.add(values, 4) == Err::OK);
    REQUIRE(writer.add_BINARY(binary, sizeof(binary)) == Err::OK);

    REQUIRE(writer.size() == sizeof(values) + sizeof(binary));

    REQUIRE(std::memcmp(buffer.data(), values, sizeof(values)) == 0);
    REQUIRE(std::memcmp(buffer.data() + sizeof(values),
                        binary, sizeof(binary)) == 0);
}

TEST_CASE("Writer: buffer overflow", "[writer]")
{
    std::array<uint8_t, 4> buffer{};
    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT32(0x12345678) == Err::OK);
    REQUIRE(writer.size() == 4);

    REQUIRE(writer.add_UINT8(0x12) == Err::BUFFER_FULL);
    REQUIRE(writer.size() == 4);
}

TEST_CASE("Writer: failed operation does not advance cursor", "[writer]")
{
    std::array<uint8_t, 4> buffer{};
    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT32(0x12345678) == Err::OK);
    REQUIRE(writer.add_UINT16(0x1234) == Err::BUFFER_FULL);
    REQUIRE(writer.size() == 4);
}

TEST_CASE("Writer: reset", "[writer]")
{
    std::array<uint8_t, 16> buffer{};
    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT32(1234) == Err::OK);
    REQUIRE(writer.size() == 4);

    writer.reset();

    REQUIRE(writer.size() == 0);
    REQUIRE(writer.add_UINT8(42) == Err::OK);
    REQUIRE(writer.size() == 1);
}

TEST_CASE("Reader: primitive types round trip", "[reader]")
{
    std::array<uint8_t, 64> buffer{};

    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT8(0x12) == Err::OK);
    REQUIRE(writer.add_INT16(-1234) == Err::OK);
    REQUIRE(writer.add_UINT32(0x12345678) == Err::OK);
    REQUIRE(writer.add_FLOAT(3.14f) == Err::OK);
    REQUIRE(writer.add_DOUBLE(3.14159265358979) == Err::OK);

    Reader reader{etl::span<const uint8_t>(buffer.data(), writer.size())};

    uint8_t u8 = 0;
    int16_t i16 = 0;
    uint32_t u32 = 0;
    float f = 0;
    double d = 0;

    REQUIRE(reader.get(u8) == Err::OK);
    REQUIRE(reader.get(i16) == Err::OK);
    REQUIRE(reader.get(u32) == Err::OK);
    REQUIRE(reader.get(f) == Err::OK);
    REQUIRE(reader.get(d) == Err::OK);

    REQUIRE(u8 == 0x12);
    REQUIRE(i16 == -1234);
    REQUIRE(u32 == 0x12345678);
    REQUIRE(f == Catch::Approx(3.14f));
    REQUIRE(d == Catch::Approx(3.14159265358979));
    REQUIRE(reader.remaining() == 0);
}

TEST_CASE("Reader: arrays and binary data", "[reader]")
{
    std::array<uint8_t, 32> buffer{};

    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    uint16_t source[] = {10, 20, 30, 40};
    uint8_t binary[] = {0xAA, 0xBB, 0xCC};

    REQUIRE(writer.add(source, 4) == Err::OK);
    REQUIRE(writer.add_BINARY(binary, sizeof(binary)) == Err::OK);

    Reader reader{etl::span<const uint8_t>(buffer.data(), writer.size())};

    uint16_t destination[4]{};
    uint8_t received[3]{};

    REQUIRE(reader.get(destination, 4) == Err::OK);
    REQUIRE(reader.get_BINARY(received, sizeof(received)) == Err::OK);

    REQUIRE(std::memcmp(source, destination, sizeof(source)) == 0);
    REQUIRE(std::memcmp(binary, received, sizeof(binary)) == 0);
    REQUIRE(reader.remaining() == 0);
}

TEST_CASE("Reader: buffer underflow", "[reader]")
{
    std::array<uint8_t, 4> buffer{};
    Reader reader{etl::span<const uint8_t>(buffer.data(), buffer.size())};

    uint32_t value = 1234;

    REQUIRE(reader.get(value) == Err::OK);
    REQUIRE(reader.get(value) == Err::BUFFER_EMPTY);
    REQUIRE(reader.remaining() == 0);
}

TEST_CASE("Reader: failed operation preserves cursor and output", "[reader]")
{
    std::array<uint8_t, 2> buffer{};
    Reader reader{etl::span<const uint8_t>(buffer.data(), buffer.size())};

    uint32_t value = 0x12345678;

    REQUIRE(reader.get(value) == Err::BUFFER_EMPTY);
    REQUIRE(reader.position() == 0);
    REQUIRE(value == 0x12345678);
}

TEST_CASE("Reader: reset and position", "[reader]")
{
    std::array<uint8_t, 16> buffer{};

    Writer writer{etl::span<uint8_t>(buffer.data(), buffer.size())};

    REQUIRE(writer.add_UINT32(1234) == Err::OK);
    REQUIRE(writer.add_UINT16(5678) == Err::OK);

    Reader reader{etl::span<const uint8_t>(buffer.data(), writer.size())};

    uint32_t a;
    uint16_t b;

    REQUIRE(reader.position() == 0);
    REQUIRE(reader.remaining() == 6);

    REQUIRE(reader.get(a) == Err::OK);
    REQUIRE(reader.position() == 4);
    REQUIRE(reader.remaining() == 2);

    reader.reset();

    REQUIRE(reader.position() == 0);
    REQUIRE(reader.remaining() == 6);

    REQUIRE(reader.get(a) == Err::OK);
    REQUIRE(reader.get(b) == Err::OK);
    REQUIRE(a == 1234);
    REQUIRE(b == 5678);
}
