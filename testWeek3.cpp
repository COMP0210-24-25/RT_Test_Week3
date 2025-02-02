#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include "VectorUtils.hpp"
#include "Rendering.hpp"
#include "Sphere.hpp"
#include "Ray.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include <string>
#include <fstream>
#include <exception>
#include <sstream>
#include <memory>
#include "ImageUtils.hpp"

using namespace Catch::Matchers;

std::vector<std::vector<std::array<float, 3>>> loadImage(std::string filename, uint w = 100, uint h = 100)
{
    std::vector<std::vector<std::array<float, 3>>> image;
    std::ifstream image_file;
    image_file.open(filename);
    if (image_file)
    {
        std::string line;

        // ignore header line
        std::getline(image_file, line);

        // get dimensions
        std::getline(image_file, line);
        std::istringstream line_stream(line);
        uint width, height;
        line_stream >> width;
        line_stream >> height;

        if ((width != w) || (height != h))
        {
            throw std::runtime_error("Dimensions of the image are not as expected");
        }

        image.resize(width);
        for (auto &col : image)
        {
            col.resize(height);
        }

        // ignore  pixel limit
        std::getline(image_file, line);

        for (uint y = 0; y < height; y++)
        {
            for (uint x = 0; x < width; x++)
            {
                if (std::getline(image_file, line))
                {
                    std::istringstream pixel_stream(line);
                    pixel_stream >> image[x][y][0];
                    pixel_stream >> image[x][y][1];
                    pixel_stream >> image[x][y][2];
                }
                else
                {
                    throw std::runtime_error("Ran out of pixel data.");
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("File " + filename + " not found.");
    }

    return image;
}

TEST_CASE("Write file with invalid name", "[Test file IO]")
{
    Sphere sphere(2, {0,0,0}, Material(255, 255, 0));
    Camera cam(100, 100);
    
    // Attempt to render
    auto image_data = Render::genImage(cam, sphere);

    REQUIRE_THROWS(SaveImage(image_data, ""));
}

TEST_CASE("Output Image with extreme values", "[Test file IO]")
{
    using Colour = std::array<float, 3>;
    constexpr Colour overflow{1000, 300, 255.5};
    constexpr Colour underflow{-0.2, -1000, -25.3};
    constexpr Colour ordinary{10.4, 1.0, 245.1};
    std::vector<std::vector<std::array<float, 3>>> Image{{overflow, underflow, ordinary}};
    
    REQUIRE_NOTHROW(SaveImage(Image, "TestImage.pbm"));

    auto loaded_image = loadImage("TestImage.pbm", 1, 3);
    assert(loaded_image.size() == 1);
    assert(loaded_image[0].size() == 3);
    Colour loaded_overflow = loaded_image[0][0];
    Colour loaded_underflow = loaded_image[0][1];
    Colour loaded_normal = loaded_image[0][2];

    for(size_t i = 0; i < 3; i++)
    {
        REQUIRE_THAT(loaded_overflow[i], WithinAbs(255.0, 1e-5));
        REQUIRE_THAT(loaded_underflow[i], WithinAbs(0.0, 1e-5));
    }
    REQUIRE_THAT(loaded_normal[0], WithinAbs(10.0, 1e-5));
    REQUIRE_THAT(loaded_normal[1], WithinAbs(1.0, 1e-5));
    REQUIRE_THAT(loaded_normal[2], WithinAbs(245.0, 1e-5));
}

TEST_CASE("Test image unchanged by file output", "[Test file IO]")
{
    using Colour = std::array<float, 3>;
    constexpr Colour overflow{1000, 300, 255.5};
    constexpr Colour underflow{-0.2, -1000, -25.3};
    constexpr Colour ordinary{10.4, 1.0, 245.1};
    std::vector<std::vector<std::array<float, 3>>> Image{{overflow, underflow, ordinary}};
    
    auto ImageCopy = Image;

    REQUIRE_NOTHROW(SaveImage(Image, "TestImage2.pbm"));

    REQUIRE(ImageCopy == Image);
}

TEST_CASE("Test negative radius", "[Test Sphere]")
{
    using namespace VecUtils;
    std::unique_ptr<Sphere> S;
    REQUIRE_THROWS(S = std::make_unique<Sphere>(-2.0, Vec3{0, 0, 0}));
}

TEST_CASE("Test width/height", "[Test Camera]")
{
    std::unique_ptr<Camera> Cam;
    unsigned int w = 100;
    unsigned int h = 50;
    REQUIRE_NOTHROW(Cam = std::make_unique<Camera>(w, h));

    REQUIRE_THROWS(Cam = std::make_unique<Camera>(0, h));
    REQUIRE_THROWS(Cam = std::make_unique<Camera>(w, 0));
}

TEST_CASE("Test Vector Norm", "[Test Vector]")
{
    using namespace VecUtils;
    REQUIRE_THROWS(norm({0,0,0}));
}

TEST_CASE("Test Object Pointer", "[Test Pointer]")
{
    // Check pointer unitialised to begin with
    IntersectionData id;
    REQUIRE_THROWS(id.getObject());

    Camera cam(1, 1);
    Material mat(255, 255, 255);
    Sphere S(1, VecUtils::Vec3{0,0,0}, mat);

    // pointer still unset if intersection misses
    S.Intersect(Ray({0,0,10}, {1, 0, 0}), id);
    REQUIRE_THROWS(id.getObject());

    // pointer set if intersection found
    S.Intersect(Ray({0,0,10}, {0, 0, -1}), id);
    REQUIRE_NOTHROW(id.getObject());
    REQUIRE(id.getObject() == &S);
}