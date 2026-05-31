#include <array>
#include <cstddef>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>

template <std::size_t N>
using Point = std::array<double, N>;

template <std::size_t N>
double distance(const Point<N>& p1, const Point<N>& p2) {
    double sum = 0;
    for (std::size_t i = 0; i < N; ++i) {
        sum += (p1[i] - p2[i]) * (p1[i] - p2[i]);
    }
    return std::sqrt(sum);
}

template <std::size_t N>
class KDTree {
public:
    explicit KDTree(std::vector<Point<N>>&& points) : points_(std::move(points)) {
        RecursiveBuild(0, 0, points_.size());
    }

    std::pair<const Point<N>&, std::size_t> GetClosest(const Point<N>& point) {
        cnt_ = 0;
        best_distance_ = std::numeric_limits<double>::max();
        RecursiveFind(0, 0, points_.size(), point);
        return {points_[best_index_], cnt_};
    }

private:
    void RecursiveBuild(std::size_t d, std::size_t l, std::size_t r) {
        if (r - l <= 1) {
            return;
        }
        const auto split_factor = GetSplitFactor(d);
        std::nth_element(points_.begin() + l, points_.begin() + (l + r) / 2, points_.begin() + r, [split_factor](const Point<N>& a, const Point<N>& b){
            return a[split_factor] < b[split_factor];
        });
        RecursiveBuild(d + 1, l, (l + r) / 2);
        RecursiveBuild(d + 1, (l + r) / 2 + 1, r);
    }

    void RecursiveFind(std::size_t d, std::size_t l, std::size_t r, const Point<N>& point) {
        ++cnt_;
        if (r - l == 1) {
            Update(point, l);
            return;
        }
        const auto split_factor = GetSplitFactor(d);
        std::size_t mid = (l + r) / 2;
        if (point[split_factor] < points_[mid][split_factor]) {
            RecursiveFind(d + 1, l, mid, point);
            if (best_distance_ < points_[mid][split_factor] - point[split_factor]) {
                return;
            }
            Update(point, mid);
            if (mid + 1 < r) {
                RecursiveFind(d + 1, mid + 1, r, point);
            }
        } else {
            if (mid + 1 < r) {
                RecursiveFind(d + 1, mid + 1, r, point);
            }
            if (best_distance_ < point[split_factor] - points_[mid][split_factor]) {
                return;
            }
            Update(point, mid);
            RecursiveFind(d + 1, l, mid, point);
        }
    }

    size_t GetSplitFactor(std::size_t d) const {
        return d % N;
    }

    void Update(const Point<N>& point, size_t index) {
        const auto dist = distance(point, points_[index]);
        if (dist < best_distance_) {
            best_distance_ = dist;
            best_index_ = index;
        }
    }

    double best_distance_;
    std::size_t best_index_;
    std::size_t cnt_;
    std::vector<Point<N>> points_;
};