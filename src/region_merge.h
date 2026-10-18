#pragma once

#include <vector>
#include <glm/glm.hpp>

// Groups neighboring panes of similar color. Panes are a cols x rows grid with
// row 0 first; each is adjacent to its 8 grid neighbors. Edges are merged from
// the most similar pair up, and two groups join only while the distance between
// their mean colors stays under the threshold, so a gradient can't chain one
// group across the whole picture. A threshold of 0 merges nothing.
//
// group[i] is the index of a pane representing pane i's group; mean[i] is that
// group's mean color.
void mergeRegions(const std::vector<glm::vec3> &colors, int cols, int rows, float threshold,
                  std::vector<int> &group, std::vector<glm::vec3> &mean);

// Luma-weighted RGB distance, 0 for equal colors and 1 for black against white.
float colorDistance(const glm::vec3 &a, const glm::vec3 &b);
