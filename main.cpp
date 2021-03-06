#include <bits/stdc++.h>
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

    vector<array<char, W> > f(H);
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
    for (int iteration = 0; ; ++ iteration) {
        if (iteration % 128 == 0) {
            chrono::high_resolution_clock::time_point clock_now = chrono::high_resolution_clock::now();
            temperature = static_cast<long double>((clock_end - clock_now).count()) / (clock_end - clock_begin).count();
            if (temperature <= 0.0) {
                fprintf(stderr, "done  (iteration = %d)\n", iteration);
                break;
            }
        }

        // pick a neighbor
        int i = uniform_int_distribution<int>(0, n - 1)(gen);
        int dir = uniform_int_distribution<int>(0, 4 - 1)(gen);
        int amount_min = -100;
        int amount_max = 100;
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
            bool found = false;
            REP3 (y, ly, ry) {
                REP3 (x, lx, rx) {
                    if (f[y][x] != -1) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (found) {
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

        auto probability = [&]() {
            constexpr double boltzmann = 3;
            return exp(boltzmann * delta) * temperature;
        };
        if (delta >= 0.0 or bernoulli_distribution(probability())(gen)) {

            // accept
            pre_score += delta;
            if (pre_highscore < pre_score) {
                // fprintf(stderr, "highscore = %d  (iteration = %d)\n", static_cast<int>(1e9 * pre_highscore / n), iteration);
                pre_highscore = pre_score;
                ans = pack_state(n, a, b, c, d);
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
        }
        // assert (static_cast<int>(1e9 * pre_score / n) == compute_score(n, x, y, r, a, b, c, d));
        // REP (j, n) {
        //     if (j != i) {
        //         assert (min(c[i], c[j]) <= max(a[i], a[j]) or min(d[i], d[j]) <= max(b[i], b[j]));
        //     }
        // }
    }
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
