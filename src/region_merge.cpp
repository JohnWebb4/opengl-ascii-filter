#include "region_merge.h"

#include <algorithm>
#include <cmath>
#include <numeric>

float colorDistance(const glm::vec3 &a, const glm::vec3 &b)
{
  glm::vec3 d = a - b;
  return std::sqrt(0.299f * d.r * d.r + 0.587f * d.g * d.g + 0.114f * d.b * d.b);
}

namespace
{
struct Edge
{
  float distance;
  int a, b;
};

struct Groups
{
  explicit Groups(const std::vector<glm::vec3> &colors) : parent(colors.size()), sum(colors), count(colors.size(), 1)
  {
    std::iota(parent.begin(), parent.end(), 0);
  }

  int find(int i)
  {
    while (parent[i] != i)
    {
      parent[i] = parent[parent[i]];
      i = parent[i];
    }
    return i;
  }

  glm::vec3 meanOf(int root) const { return sum[root] / static_cast<float>(count[root]); }

  std::vector<int> parent;
  std::vector<glm::vec3> sum;
  std::vector<int> count;
};
}  // namespace

void mergeRegions(const std::vector<glm::vec3> &colors, int cols, int rows, float threshold,
                  std::vector<int> &group, std::vector<glm::vec3> &mean)
{
  const int total = cols * rows;
  Groups groups(colors);

  if (threshold > 0.0f)
  {
    // Each pair once: right, down, and both diagonals below.
    std::vector<Edge> edges;
    edges.reserve(static_cast<size_t>(total) * 4);
    for (int y = 0; y < rows; ++y)
    {
      for (int x = 0; x < cols; ++x)
      {
        const int i = y * cols + x;
        const int neighbors[4][2] = {{x + 1, y}, {x, y + 1}, {x + 1, y + 1}, {x - 1, y + 1}};
        for (const auto &n : neighbors)
        {
          if (n[0] >= 0 && n[0] < cols && n[1] < rows)
          {
            const int j = n[1] * cols + n[0];
            edges.push_back({colorDistance(colors[i], colors[j]), i, j});
          }
        }
      }
    }
    std::sort(edges.begin(), edges.end(), [](const Edge &l, const Edge &r) { return l.distance < r.distance; });

    for (const Edge &edge : edges)
    {
      const int ra = groups.find(edge.a);
      const int rb = groups.find(edge.b);
      if (ra != rb && colorDistance(groups.meanOf(ra), groups.meanOf(rb)) < threshold)
      {
        groups.parent[rb] = ra;
        groups.sum[ra] += groups.sum[rb];
        groups.count[ra] += groups.count[rb];
      }
    }
  }

  group.resize(total);
  mean.resize(total);
  for (int i = 0; i < total; ++i)
  {
    group[i] = groups.find(i);
    mean[i] = groups.meanOf(group[i]);
  }
}
