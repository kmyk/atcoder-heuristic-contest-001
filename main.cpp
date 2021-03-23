#include <bits/stdc++.h>
#ifdef VISUALIZE
#include "vis.hpp"
#endif
#define REP(i, n) for (int i = 0; (i) < (int)(n); ++ (i))
#define REP3(i, m, n) for (int i = (m); (i) < (int)(n); ++ (i))
#define REP_R(i, n) for (int i = (int)(n) - 1; (i) >= 0; -- (i))
#define REP3R(i, m, n) for (int i = (int)(n) - 1; (i) >= (int)(m); -- (i))
#define ALL(x) std::begin(x), std::end(x)
#define dump(x) cerr << #x " = " << x << endl
using namespace std;
template <class T> using reversed_priority_queue = priority_queue<T, vector<T>, greater<T> >;
template <class T, class U> inline void chmax(T & a, U const & b) { a = max<T>(a, b); }
template <class T, class U> inline void chmin(T & a, U const & b) { a = min<T>(a, b); }
template <typename T> ostream & operator << (ostream & out, vector<T> const & xs) { REP (i, (int)xs.size() - 1) out << xs[i] << ' '; if (not xs.empty()) out << xs.back(); return out; }

class xor_shift_128 {
public:
    typedef uint32_t result_type;
    xor_shift_128(uint32_t seed = 42) {
        set_seed(seed);
    }
    void set_seed(uint32_t seed) {
        a = seed = 1812433253u * (seed ^ (seed >> 30));
        b = seed = 1812433253u * (seed ^ (seed >> 30)) + 1;
        c = seed = 1812433253u * (seed ^ (seed >> 30)) + 2;
        d = seed = 1812433253u * (seed ^ (seed >> 30)) + 3;
    }
    uint32_t operator() () {
        uint32_t t = (a ^ (a << 11));
        a = b; b = c; c = d;
        return d = (d ^ (d >> 19)) ^ (t ^ (t >> 8));
    }
    static constexpr uint32_t max() { return numeric_limits<result_type>::max(); }
    static constexpr uint32_t min() { return numeric_limits<result_type>::min(); }
private:
    uint32_t a, b, c, d;
};

constexpr int H = 10000;
constexpr int W = 10000;

constexpr int UP = 0;
constexpr int DOWN = 1;
constexpr int RIGHT = 2;
constexpr int LEFT = 3;

vector<tuple<int, int, int, int> > pack_state(int n, const vector<int>& a, const vector<int>& b, const vector<int>& c, const vector<int>& d) {
    vector<tuple<int, int, int, int> > ans(n);
    REP (i, n) {
        ans[i] = make_tuple(a[i], b[i], c[i], d[i]);
    }
    return ans;
}

int compute_score(int n, const vector<int>& x, const vector<int>& y, const vector<int>& r, const vector<int>& a, const vector<int>& b, const vector<int>& c, const vector<int>& d) {
    long double score = 0;
    REP (i, n) {
        assert (0 <= a[i] and a[i] <= x[i] and x[i] < c[i] and c[i] <= W);
        assert (0 <= b[i] and b[i] <= y[i] and y[i] < d[i] and d[i] <= H);
        int s = (c[i] - a[i]) * (d[i] - b[i]);
        long double q = static_cast<long double>(min(r[i], s)) / max(r[i], s);
        score += 2 * q - powl(q, 2);
    }
    return 1e9 * score / n;
}

#ifdef VISUALIZE
struct sidebar_info {
    int iteration;
    double temperature;
    int score;
    int highscore;
};
#endif

template <typename RandomEngine>
vector<tuple<int, int, int, int> > solve(int n, const vector<int>& x, const vector<int>& y, const vector<int>& r, RandomEngine& gen, chrono::high_resolution_clock::time_point clock_end) {
    chrono::high_resolution_clock::time_point clock_begin = chrono::high_resolution_clock::now();

    // state
    vector<int> a(n, -1);
    vector<int> b(n, -1);
    vector<int> c(n, -1);
    vector<int> d(n, -1);
    REP (i, n) {
        a[i] = x[i];
        b[i] = y[i];
        c[i] = x[i] + 1;
        d[i] = y[i] + 1;
    }

    // area
    auto get_pre_score = [&](int i) -> long double {
        assert (0 <= a[i] and a[i] <= x[i] and x[i] < c[i] and c[i] <= W);
        assert (0 <= b[i] and b[i] <= y[i] and y[i] < d[i] and d[i] <= H);
        int s = (c[i] - a[i]) * (d[i] - b[i]);
        long double q = static_cast<long double>(min(r[i], s)) / max(r[i], s);
        return 2 * q - powl(q, 2);
    };
    long double pre_score = 0;
    REP (i, n) {
        pre_score += get_pre_score(i);
    }

    vector<array<uint8_t, W> > f(H);
    REP (y, H) {
        REP (x, W) {
            f[y][x] = -1;
        }
    }
    REP (i, n) {
        f[y[i]][x[i]] = i;
    }

    vector<tuple<int, int, int, int> > ans = pack_state(n, a, b, c, d);
    long double pre_highscore = pre_score;

    // simulated annealing
    double temperature = 1.0;
    int iteration = 0;

#ifdef VISUALIZE
    deque<visualizer::image> movie;
    deque<sidebar_info> sidebar;
    const int SCREEN_SIZE = 1000;
    const int SIDEBAR_WIDTH = 200;
    const double SCALE = SCREEN_SIZE / (double)H;
    auto render = [&]() {
        visualizer::image g(SCREEN_SIZE, SCREEN_SIZE + SIDEBAR_WIDTH);
        g.add_rect(1, 1, SCREEN_SIZE - 2, SCREEN_SIZE - 2, visualizer::BLACK);
        REP (i, n) {
            double p = get_pre_score(i);
            visualizer::color stroke { 1.0 - p, 0.2, p * 0.8, 1.0 };
            visualizer::color fill { 1.0 - p, 0.2, p * 0.8, 1.0 - p };
            g.add_rect(a[i] * SCALE, b[i] * SCALE, (c[i] - a[i]) * SCALE, (d[i] - b[i]) * SCALE, stroke, fill);
            g.add_line((a[i] + c[i]) / 2.0 * SCALE, (b[i] + d[i]) / 2.0 * SCALE, x[i] * SCALE, y[i] * SCALE, stroke);
            char buf[64];
            sprintf(buf, "%.3f", p);
            g.add_text((a[i] + c[i]) / 2.0 * SCALE, (b[i] + d[i]) / 2.0 * SCALE, string(buf), visualizer::BLACK, 12, "middle");
        }
        movie.push_back(move(g));

        sidebar_info info;
        info.iteration = iteration;
        info.temperature = temperature;
        info.score = 1e9 * pre_score / n;
        info.highscore = 1e9 * pre_highscore / n;
        sidebar.push_back(info);
    };
#else
    auto render = []() {
    };
#endif

    for (; ; ++ iteration) {
        if (iteration % 32 == 0) {
            chrono::high_resolution_clock::time_point clock_now = chrono::high_resolution_clock::now();
            temperature = static_cast<long double>((clock_end - clock_now).count()) / (clock_end - clock_begin).count();
            if (temperature <= 0.0) {
                fprintf(stderr, "done  (iteration = %d)\n", iteration);
                break;
            }
            render();
        }

        // pick a neighbor
        int i = uniform_int_distribution<int>(0, n - 1)(gen);
        int dir = uniform_int_distribution<int>(0, 4 - 1)(gen);
        int amount_min = -100;
        int amount_max = 300;
        if (dir == LEFT) {
            amount_max = min(amount_max, a[i]);
            amount_min = max(amount_min, - (x[i] - a[i]));
        } else if (dir == UP) {
            amount_max = min(amount_max, b[i]);
            amount_min = max(amount_min, - (y[i] - b[i]));
        } else if (dir == RIGHT) {
            amount_max = min(amount_max, W - c[i]);
            amount_min = max(amount_min, - (c[i] - x[i]) + 1);
        } else if (dir == DOWN) {
            amount_max = min(amount_max, H - d[i]);
            amount_min = max(amount_min, - (d[i] - y[i]) + 1);
        } else {
            assert (false);
        }
        assert (amount_min <= amount_max);
        if (amount_min == 0 and amount_max == 0) {
            continue;
        }
        int amount = 0;
        while (amount == 0) {
            amount = uniform_int_distribution<int>(amount_min, amount_max)(gen);
        }

        // check
        set<int> overlap;
        if (amount > 0) {
            int ly = b[i];
            int ry = d[i];
            int lx = a[i];
            int rx = c[i];
            if (dir == LEFT) {
                lx = a[i] - amount;
                rx = a[i];
            } else if (dir == UP) {
                ly = b[i] - amount;
                ry = b[i];
            } else if (dir == RIGHT) {
                lx = c[i];
                rx = c[i] + amount;
            } else if (dir == DOWN) {
                ly = d[i];
                ry = d[i] + amount;
            } else {
                assert (false);
            }
            assert (0 <= ly and ly < ry and ry <= H);
            assert (0 <= lx and lx < rx and rx <= W);
            REP3 (y, ly, ry) {
                REP3 (x, lx, rx) {
                    if (f[y][x] != static_cast<uint8_t>(-1)) {
                        overlap.insert(f[y][x]);
                        x = c[f[y][x]] - 1;
                    }
                }
            }
        }
        if (not overlap.empty()) {
            int ly = b[i];
            int ry = d[i];
            int lx = a[i];
            int rx = c[i];
            assert (amount > 0);
            if (dir == LEFT) {
                lx = a[i] - amount;
            } else if (dir == UP) {
                ly = b[i] - amount;
            } else if (dir == RIGHT) {
                rx = c[i] + amount;
            } else if (dir == DOWN) {
                ry = d[i] + amount;
            } else {
                assert (false);
            }
            bool impossible = false;
            for (int j : overlap) {
                assert (0 <= j and j < n);
                if (ly <= y[j] and y[j] < ry and lx <= x[j] and x[j] < rx) {
                    impossible = true;
                    break;
                }
            }
            if (impossible) {
                continue;
            }
        }

        // update
        long double delta = 0;
        delta -= get_pre_score(i);
        if (dir == LEFT) {
            a[i] -= amount;
        } else if (dir == UP) {
            b[i] -= amount;
        } else if (dir == RIGHT) {
            c[i] += amount;
        } else if (dir == DOWN) {
            d[i] += amount;
        } else {
            assert (false);
        }
        delta += get_pre_score(i);
        if (dir == LEFT) {
            a[i] += amount;
        } else if (dir == UP) {
            b[i] += amount;
        } else if (dir == RIGHT) {
            c[i] -= amount;
        } else if (dir == DOWN) {
            d[i] -= amount;
        } else {
            assert (false);
        }
        map<int, tuple<int, int, int, int> > preserved_overlap;
        for (int j : overlap) {
            assert (j != i);
            delta -= get_pre_score(j);
            preserved_overlap[j] = make_tuple(a[j], b[j], c[j], d[j]);
            assert (amount > 0);
            if (dir == LEFT) {
                if (x[j] < a[i] - amount) {
                    c[j] = a[i] - amount;
                } else if (y[j] < b[i]) {
                    d[j] = b[i];
                } else if (c[i] <= x[j]) {
                    a[j] = c[i];
                } else if (d[i] <= y[j]) {
                    b[j] = d[i];
                } else {
                    assert (false);
                }
                assert (min(c[i], c[j]) <= max(a[i] - amount, a[j]) or min(d[i], d[j]) <= max(b[i], b[j]));
            } else if (dir == UP) {
                if (x[j] < a[i]) {
                    c[j] = a[i];
                } else if (y[j] < b[i] - amount) {
                    d[j] = b[i] - amount;
                } else if (c[i] <= x[j]) {
                    a[j] = c[i];
                } else if (d[i] <= y[j]) {
                    b[j] = d[i];
                } else {
                    assert (false);
                }
                assert (min(c[i], c[j]) <= max(a[i], a[j]) or min(d[i], d[j]) <= max(b[i] - amount, b[j]));
            } else if (dir == RIGHT) {
                if (x[j] < a[i]) {
                    c[j] = a[i];
                } else if (y[j] < b[i]) {
                    d[j] = b[i];
                } else if (c[i] + amount <= x[j]) {
                    a[j] = c[i] + amount;
                } else if (d[i] <= y[j]) {
                    b[j] = d[i];
                } else {
                    assert (false);
                }
                assert (min(c[i] + amount, c[j]) <= max(a[i], a[j]) or min(d[i], d[j]) <= max(b[i], b[j]));
            } else if (dir == DOWN) {
                if (x[j] < a[i]) {
                    c[j] = a[i];
                } else if (y[j] < b[i]) {
                    d[j] = b[i];
                } else if (c[i] <= x[j]) {
                    a[j] = c[i];
                } else if (d[i] + amount <= y[j]) {
                    b[j] = d[i] + amount;
                } else {
                    assert (false);
                }
                assert (min(c[i], c[j]) <= max(a[i], a[j]) or min(d[i] + amount, d[j]) <= max(b[i], b[j]));
            } else {
                assert (false);
            }
            assert (0 <= a[j] and a[j] <= x[j] and x[j] < c[j] and c[j] <= W);
            assert (0 <= b[j] and b[j] <= y[j] and y[j] < d[j] and d[j] <= H);
            delta += get_pre_score(j);
        }

        auto probability = [&]() {
            constexpr long double boltzmann = 100;
            return exp(boltzmann * delta) * temperature;
        };
        if (delta >= 0.0 or bernoulli_distribution(probability())(gen)) {

            // accept
            if (delta < -1e-9) {
                fprintf(stderr, "decreasing move  (delta = %Lf, iteration = %d)\n", delta, iteration);
            }
            pre_score += delta;
            if (pre_highscore < pre_score) {
                fprintf(stderr, "highscore = %d  (iteration = %d)\n", static_cast<int>(1e9 * pre_highscore / n), iteration);
                pre_highscore = pre_score;
                ans = pack_state(n, a, b, c, d);
            }

            for (auto [j, preserved] : preserved_overlap) {
                auto [a_j, b_j, c_j, d_j] = preserved;
                for (; a_j < a[j]; ++ a_j) {
                    REP3 (y, b_j, d_j) {
                        f[y][a_j] = -1;
                    }
                }
                for (; b_j < b[j]; ++ b_j) {
                    REP3 (x, a_j, c_j) {
                        f[b_j][x] = -1;
                    }
                }
                for (; c[j] < c_j; -- c_j) {
                    REP3 (y, b_j, d_j) {
                        f[y][c_j - 1] = -1;
                    }
                }
                for (; d[j] < d_j; -- d_j) {
                    REP3 (x, a_j, c_j) {
                        f[d_j - 1][x] = -1;
                    }
                }
                assert (a_j == a[j]);
                assert (b_j == b[j]);
                assert (c_j == c[j]);
                assert (d_j == d[j]);
            }

            if (amount > 0) {
                int ly = b[i];
                int ry = d[i];
                int lx = a[i];
                int rx = c[i];
                if (dir == LEFT) {
                    lx = a[i] - amount;
                    rx = a[i];
                } else if (dir == UP) {
                    ly = b[i] - amount;
                    ry = b[i];
                } else if (dir == RIGHT) {
                    lx = c[i];
                    rx = c[i] + amount;
                } else if (dir == DOWN) {
                    ly = d[i];
                    ry = d[i] + amount;
                } else {
                    assert (false);
                }
                REP3 (y, ly, ry) {
                    REP3 (x, lx, rx) {
                        f[y][x] = i;
                        assert (0 <= f[y][x] and f[y][x] < n);
                    }
                }
            } else {
                int ly = b[i];
                int ry = d[i];
                int lx = a[i];
                int rx = c[i];
                if (dir == LEFT) {
                    lx = a[i];
                    rx = a[i] - amount;
                } else if (dir == UP) {
                    ly = b[i];
                    ry = b[i] - amount;
                } else if (dir == RIGHT) {
                    lx = c[i] + amount;
                    rx = c[i];
                } else if (dir == DOWN) {
                    ly = d[i] + amount;
                    ry = d[i];
                } else {
                    assert (false);
                }
                REP3 (y, ly, ry) {
                    REP3 (x, lx, rx) {
                        f[y][x] = -1;
                    }
                }
            }

            if (dir == LEFT) {
                a[i] -= amount;
            } else if (dir == UP) {
                b[i] -= amount;
            } else if (dir == RIGHT) {
                c[i] += amount;
            } else if (dir == DOWN) {
                d[i] += amount;
            } else {
                assert (false);
            }

        } else {
            // reject
            for (auto [j, preserved] : preserved_overlap) {
                tie(a[j], b[j], c[j], d[j]) = preserved;
            }
        }
        // assert (static_cast<int>(1e9 * pre_score / n) == compute_score(n, x, y, r, a, b, c, d));
        // REP (j, n) {
        //     assert (0 <= a[j] and a[j] <= x[j] and x[j] < c[j] and c[j] <= W);
        //     assert (0 <= b[j] and b[j] <= y[j] and y[j] < d[j] and d[j] <= H);
        //     if (j != i) {
        //         assert (min(c[i], c[j]) <= max(a[i], a[j]) or min(d[i], d[j]) <= max(b[i], b[j]));
        //     }
        // }
    }

#ifdef VISUALIZE
    if (movie.size() > 1000) {
        std::deque<visualizer::image> selected_movie;
        std::deque<sidebar_info> selected_sidebar;
        REP (i, movie.size()) {
            if (bernoulli_distribution(1000.0 / movie.size())(gen)) {
                selected_movie.push_back(move(movie[i]));
                selected_sidebar.push_back(sidebar[i]);
            }
        }
        movie.swap(selected_movie);
        sidebar.swap(selected_sidebar);
    }
    int total_iteration = iteration;
    int highscore = 1e9 * pre_highscore / n;
    REP (i, movie.size()) {
        int LINE_HEIGHT = 16;
        int FONT_SIZE = 16;
        int PADDING = 12;
        movie[i].add_text(LINE_HEIGHT, SCREEN_SIZE + PADDING, "iteration: " + to_string(sidebar[i].iteration) + " / " + to_string(total_iteration), visualizer::BLACK, FONT_SIZE);
        movie[i].add_text(LINE_HEIGHT * 2, SCREEN_SIZE + PADDING, "temperature: " + to_string(sidebar[i].temperature), visualizer::BLACK, FONT_SIZE);
        movie[i].add_text(LINE_HEIGHT * 3, SCREEN_SIZE + PADDING, "score: " + to_string(sidebar[i].score), visualizer::BLACK, FONT_SIZE);
        movie[i].add_text(LINE_HEIGHT * 4, SCREEN_SIZE + PADDING, "highscore: " + to_string(sidebar[i].highscore) + " / " + to_string(highscore), visualizer::BLACK, FONT_SIZE);

        int OFFSET_Y = LINE_HEIGHT * 5;
        auto get_y_from_iteration = [&](int iteration) {
            return iteration / static_cast<long double>(total_iteration) * (SCREEN_SIZE - OFFSET_Y) + OFFSET_Y;
        };
        auto get_x_from_ratio = [&](long double p) {
            return p * (SIDEBAR_WIDTH - 2 * PADDING) + SCREEN_SIZE + PADDING;
        };
        movie[i].add_rect(get_y_from_iteration(0), get_x_from_ratio(0), get_y_from_iteration(total_iteration), get_x_from_ratio(1));

        vector<pair<int, int> > iteration_path;
        vector<pair<int, int> > score_path;
        vector<pair<int, int> > highscore_path;
        int y = get_y_from_iteration(0);
        int x = get_x_from_ratio(0);
        iteration_path.emplace_back(y, x);
        score_path.emplace_back(y, x);
        highscore_path.emplace_back(y, x);
        REP (j, i + 1) {
            int y = get_y_from_iteration(sidebar[j].iteration);
            int x = get_x_from_ratio(sidebar[j].iteration / static_cast<long double>(total_iteration));
            iteration_path.emplace_back(y, x);
            x = get_x_from_ratio(sidebar[j].score / static_cast<long double>(highscore));
            score_path.emplace_back(y, x);
            x = get_x_from_ratio(sidebar[j].highscore / static_cast<long double>(highscore));
            highscore_path.emplace_back(y, x);
        }
        iteration_path.erase(unique(ALL(iteration_path)), iteration_path.end());
        score_path.erase(unique(ALL(score_path)), score_path.end());
        highscore_path.erase(unique(ALL(highscore_path)), highscore_path.end());
        movie[i].add_path(iteration_path);
        movie[i].add_path(score_path, visualizer::RED);
        movie[i].add_path(highscore_path, visualizer::BLUE);
        y = get_y_from_iteration(sidebar[i].iteration);
        x = get_x_from_ratio(sidebar[i].iteration / static_cast<long double>(total_iteration));
        movie[i].add_text(y, x, "iteration");
        x = get_x_from_ratio(sidebar[i].score / static_cast<long double>(highscore));
        movie[i].add_text(y, x, "score", visualizer::RED);
        x = get_x_from_ratio(sidebar[i].highscore / static_cast<long double>(highscore));
        movie[i].add_text(y, x, "highscore", visualizer::BLUE);
    }
    visualizer::write_movie_to_file("movie.html", movie);
#endif
    return ans;
}

int main() {
    constexpr auto TIME_LIMIT = chrono::milliseconds(5000);
    chrono::high_resolution_clock::time_point clock_begin = chrono::high_resolution_clock::now();
    xor_shift_128 gen(20210306);

    int n; scanf("%d", &n);
    vector<int> x(n), y(n), r(n);
    REP (i, n) {
        scanf("%d%d%d", &x[i], &y[i], &r[i]);
    }
    auto ans = solve(n, x, y, r, gen, clock_begin + chrono::duration_cast<chrono::milliseconds>(TIME_LIMIT * 0.95));
    assert (ans.size() == n);
    for (auto [a, b, c, d] : ans) {
        printf("%d %d %d %d\n", a, b, c, d);
    }
    return 0;
}
