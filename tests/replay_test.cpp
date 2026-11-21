#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "TestSupport.h"
#include "../Inmoov_EMG/Decision.h"
#include "../Inmoov_EMG/Motion.h"

namespace {
constexpr unsigned ROW_COUNT = 60;
constexpr unsigned ROW_DURATION_MS = 500;
constexpr unsigned ROWS_PER_STAGE = 10;
const uint16_t STAGE_POSITIONS[] = {0, 500, 1000, 0, 1000, 0};

std::vector<std::string> split(const std::string &line) {
  std::istringstream stream(line);
  std::vector<std::string> fields;
  std::string field;
  while (std::getline(stream, field, ',')) fields.push_back(field);
  return fields;
}
}

int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream input(argv[1]);
  assert(input.is_open());
  std::string line;
  assert(bool(std::getline(input, line)));
  const auto header = split(line);
  assert(header.size() == 22 && header[2] == "device_ms" &&
         header[6] == "envelope_adc" && header[9] == "flags");
  Hand::ThreeLevelControl levels;
  Hand::Motion motion;
  unsigned rows = 0;
  uint32_t now = 0;
  while (std::getline(input, line)) {
    const auto fields = split(line);
    assert(rows < ROW_COUNT && fields.size() == header.size());
    assert(std::stoul(fields[2]) == rows * ROW_DURATION_MS);
    assert(std::stoul(fields[9]) == 0);
    const unsigned envelope = unsigned(std::stoul(fields[6]));
    assert(envelope <= 1023);
    // Retencion artificial: no reconstruye muestras del sensor ni su filtro.
    for (unsigned step = 0; step < ROW_DURATION_MS; ++step) {
      ++now;
      motion.requestPosition(levels.add(uint16_t(envelope), now), now);
      motion.update(now);
    }
    ++rows;
    if (rows % ROWS_PER_STAGE == 0) {
      const uint16_t expected = STAGE_POSITIONS[rows / ROWS_PER_STAGE - 1];
      assert(levels.ready() && levels.position() == expected);
      assert(motion.position() == expected);
    }
  }
  assert(input.eof() && rows == ROW_COUNT && now == 30000);
  result("REPLAY-30S", "CSV sintetico retenido 500 ms por fila, sin adquisicion ni filtro",
         "30 s simulados; destinos y rampas 0/90/180/0/180/0 completos", true);
}
