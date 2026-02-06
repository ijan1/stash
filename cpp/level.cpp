// A little toy demo implementing the minicraft level generator
// 'r' to generate a new level, 'q' to quit
// To compile:
// g++ -O3 -std=c++23 level.cpp -lSDL2 -lSDL2_image -lsfml-graphics
#include <chrono>
#include <iostream>
#include <random>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SFML/Graphics.hpp>
#include <vector>

class Tile {
public:
  static const unsigned char water = 1;
  static const unsigned char grass = 2;
  static const unsigned char rock = 3;
  static const unsigned char dirt = 4;
  static const unsigned char sand = 5;
  static const unsigned char tree = 6;
  static const unsigned char lava = 7;
  static const unsigned char cloud = 8;
  static const unsigned char stairsDown = 9;
  static const unsigned char cloudCactus = 10;
  static const unsigned char infiniteFall = 11;
  static const unsigned char flower = 12;
  static const unsigned char cactus = 13;
  static const unsigned char ironOre = 14;
};

int mod(int x, int m) { return (x % m + m) % m; }

auto random_num(int x) -> float { return (x >> 8) * 0x1.0p-23; }

#define RNG std::mt19937

class LevelGen {
private:
  std::vector<float> values;
  uint32_t w, h;
  RNG rng;

  auto sample(uint32_t x, uint32_t y) -> float {
    return values[mod(x, w) + mod(y, h) * w];
  }

  void setSample(uint32_t x, uint32_t y, float value) {
    values[mod(x, w) + mod(y, h) * w] = value;
  }

public:
  LevelGen(uint32_t w, uint32_t h, uint32_t featureSize)
      : values(w * h), w(w), h(h), rng(std::random_device{}()) {
    for (int y = 0; y < w; y += featureSize) {
      for (int x = 0; x < w; x += featureSize) {
        setSample(x, y, random_num(rng()) * 2 - 1);
      }
    }

    auto stepSize = featureSize;
    double scale = 1.0 / w;
    double scaleMod = 1;

    do {
      uint32_t halfStep = stepSize / 2;
      for (auto y = 0U; y < w; y += stepSize) {
        for (auto x = 0U; x < w; x += stepSize) {
          double a = sample(x, y);
          double b = sample(x + stepSize, y);
          double c = sample(x, y + stepSize);
          double d = sample(x + stepSize, y + stepSize);

          double e = (a + b + c + d) / 4.0 +
                     (random_num(rng()) * 2 - 1) * stepSize * scale;
          setSample(x + halfStep, y + halfStep, e);
        }
      }

      for (auto y = 0U; y < w; y += stepSize) {
        for (auto x = 0U; x < w; x += stepSize) {
          double a = sample(x, y);
          double b = sample(x + stepSize, y);
          double c = sample(x, y + stepSize);
          double d = sample(x + halfStep, y + halfStep);
          double e = sample(x + halfStep, y - halfStep);
          double f = sample(x - halfStep, y + halfStep);

          double H = (a + b + d + e) / 4.0 +
                     (random_num(rng()) * 2 - 1) * stepSize * scale * 0.5;
          double g = (a + c + d + f) / 4.0 +
                     (random_num(rng()) * 2 - 1) * stepSize * scale * 0.5;
          setSample(x + halfStep, y, H);
          setSample(x, y + halfStep, g);
        }
      }
      stepSize /= 2;
      scale *= (scaleMod + 0.8);
      scaleMod *= 0.3;
    } while (stepSize > 1);
  }

  static std::unique_ptr<unsigned char[]> createTopMap(uint32_t w, uint32_t h) {
    LevelGen mnoise1(w, h, 16);
    LevelGen mnoise2(w, h, 16);
    LevelGen mnoise3(w, h, 16);
    LevelGen noise1(w, h, 32);
    LevelGen noise2(w, h, 32);

    auto map = std::make_unique<unsigned char[]>(w * h);

    for (auto y = 0U; y < h; ++y) {
      for (auto x = 0U; x < w; ++x) {
        auto i = x + y * w;

        double val = std::abs(noise1.values[i] - noise2.values[i]) * 3 - 2;
        double mval = std::abs(mnoise1.values[i] - mnoise2.values[i]);
        mval = std::abs(mval - mnoise3.values[i]) * 3 - 2;

        double xd = x / (w - 1.0) * 2 - 1;
        double yd = y / (h - 1.0) * 2 - 1;
        if (xd < 0)
          xd = -xd;
        if (yd < 0)
          yd = -yd;
        double dist = std::max(xd, yd);
        dist = std::pow(dist, 8);

        val = val + 1 - dist * 20;

        if (val < -0.5) {
          map[i] = Tile::water;
        } else if (val > 0.5 && mval < -1.5) {
          map[i] = Tile::rock;
        } else {
          map[i] = Tile::grass;
        }
      }
    }

    RNG r(std::rand());
    std::uniform_int_distribution<uint32_t> widthdist(0, w);
    std::uniform_int_distribution<uint32_t> heightdist(0, h);
    for (auto i = 0U; i < w * h / 3000; i++) {
      auto xs = widthdist(r);
      auto ys = heightdist(r);
      for (auto k = 0U; k < 10; k++) {
        std::uniform_int_distribution<uint32_t> twentyone(0, 21);
        uint32_t x = xs + twentyone(r) - 10;
        uint32_t y = ys + twentyone(r) - 10;
        for (auto j = 0U; j < 100; j++) {
          std::uniform_int_distribution<int32_t> five(0, 5);
          int32_t xo = x + five(r) - five(r);
          int32_t yo = y + five(r) - five(r);
          for (int32_t yy = yo - 1; yy <= yo + 1; yy++) {
            for (int32_t xx = xo - 1; xx <= xo + 1; xx++) {
              if (xx >= 0 && yy >= 0 && std::cmp_less(xx , w) && std::cmp_less(yy , h)) {
                if (map[xx + yy * w] == Tile::grass) {
                  map[xx + yy * w] = Tile::sand;
                }
              }
            }
          }
        }
      }
    }

    for (auto i = 0U; i < w * h / 400; i++) {
      auto x = widthdist(r);
      auto y = widthdist(r);
      for (int j = 0; j < 200; j++) {
        std::uniform_int_distribution<uint32_t> fifteen(0, 15);
        auto xx = x + fifteen(r) - fifteen(r);
        auto yy = y + fifteen(r) - fifteen(r);
        if (xx >= 0 && yy >= 0 && xx < w && yy < h) {
          if (map[xx + yy * w] == Tile::grass) {
            map[xx + yy * w] = Tile::tree;
          }
        }
      }
    }

    for (auto i = 0U; i < w * h / 400; i++) {
      auto x = widthdist(r);
      auto y = heightdist(r);
      for (auto j = 0; j < 30; j++) {
        std::uniform_int_distribution<uint32_t> five(0, 5);
        uint32_t xx = x + five(r) - five(r);
        uint32_t yy = y + five(r) - five(r);
        if (xx >= 0 && yy >= 0 && xx < w && yy < h) {
          if (map[xx + yy * w] == Tile::grass) {
            map[xx + yy * w] = Tile::flower;
          }
        }
      }
    }

    for (auto i = 0U; i < w * h / 100; i++) {
      auto x = widthdist(r);
      auto y = heightdist(r);
      if (x >= 0 and y >= 0 and x < w and y < h) {
        if (map[x + y * w] == Tile::sand) {
          map[x + y * w] = Tile::cactus;
        }
      }
    }

    return map;
  }
};

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
uint32_t width = 1028;
uint32_t height = 1028;

void init_sdl() {
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_PNG | IMG_INIT_TIF |
           IMG_INIT_WEBP);
  window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, width, height,
                            SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

  if (window == nullptr)
    exit(2);
  //   std::cout << "failed to create window.\n";
  if (renderer == nullptr)
    exit(3);
  //   std::cout << "failed to create renderer.\n";
}

auto create_image(uint32_t w, uint32_t h) -> sf::Image {
  using namespace std::chrono;
  auto begin = steady_clock::now();

  auto map = LevelGen::createTopMap(w, h);
  auto end = steady_clock::now();
  auto total_time = end - begin;
  if (total_time >= 1ms) {
    printf("Took us %ld ms\n", duration_cast<milliseconds>(total_time).count());
  } else if (total_time >= 1us) {
    printf("Took us %ld us\n", duration_cast<microseconds>(total_time).count());
  } else {
    printf("Took us %ld ns\n", total_time.count());
  }

  sf::Image img{sf::Vector2u{w,h}};

  for (auto y = 0U; y < h; y++) {
    for (auto x = 0U; x < w; x++) {
      auto i = x + y * w;
      sf::Color color;

      switch (map[i]) {
      case Tile::water:
        color = sf::Color(0x00, 0x00, 0x80);
        break;
      case Tile::grass:
        color = sf::Color(0x20, 0x80, 0x20);
        break;
      case Tile::rock:
        color = sf::Color(0xa0, 0xa0, 0xa0);
        break;
      case Tile::dirt:
        color = sf::Color(0x60, 0x40, 0x40);
        break;
      case Tile::sand:
        color = sf::Color(0xa0, 0xa0, 0x40);
        break;
      case Tile::tree:
        color = sf::Color(0x00, 0x30, 0x00);
        break;
      case Tile::lava:
        color = sf::Color(0xff, 0x20, 0x20);
        break;
      case Tile::cloud:
        color = sf::Color(0xa0, 0xa0, 0xa0);
        break;
      case Tile::flower:
        color = sf::Color::Magenta;
        break;
      case Tile::stairsDown:
        color = sf::Color(0xff, 0xff, 0xff);
        break;
      }
      img.setPixel(sf::Vector2u{x, y}, color);
    }
  }

  return img;
}

static int get_pitch(uint32_t format, uint32_t width) {
  int pitch;

  if (SDL_ISPIXELFORMAT_FOURCC(format) || SDL_BITSPERPIXEL(format) >= 8) {
    pitch = (width * SDL_BYTESPERPIXEL(format));
  } else {
    pitch = ((width * SDL_BITSPERPIXEL(format)) + 7) / 8;
  }
  pitch = (pitch + 3) & ~3; // byte-align
  return pitch;
}

SDL_Surface *load_image(const Uint8 *buff, int w, int h) {
  SDL_Surface *temp = SDL_CreateRGBSurfaceWithFormatFrom(
      const_cast<Uint8 *>(buff), w, h, 0, get_pitch(SDL_PIXELFORMAT_RGBA32, w),
      SDL_PIXELFORMAT_RGBA32);

  if (temp == NULL) {
    // std::cout << "Failed to create surface\n";
    // std::cout << SDL_GetError() << '\n';
    exit(1);
  }
  return temp;
}

int main(int argc, char *argv[]) {
  init_sdl();

  int width = 128;
  int height = 128;

  std::srand(std::chrono::system_clock::now().time_since_epoch().count());

  SDL_Surface *surf = nullptr;
  SDL_Texture *tex = nullptr;
  {
    sf::Image img = create_image(width, height);
    surf = load_image(img.getPixelsPtr(), img.getSize().x, img.getSize().y);
    tex = SDL_CreateTextureFromSurface(renderer, surf);
  }

  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          return 0;

        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_r) {
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);

            sf::Image img = create_image(width, height);
            surf = load_image(img.getPixelsPtr(), img.getSize().x,
                              img.getSize().y);
            tex = SDL_CreateTextureFromSurface(renderer, surf);
          }

          if (event.key.keysym.sym == SDLK_q) {
            return 0;
          }
      }
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, tex, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  return 0;
}

