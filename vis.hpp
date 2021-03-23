#pragma once
#include <deque>
#include <fstream>
#include <sstream>
#include <string>

namespace visualizer {

struct color {
    double red, green, blue, alpha;
};

const color BLACK = { 0, 0, 0, 1 };
const color WHITE = { 1, 1, 1, 1 };
const color RED = { 1, 0, 0, 1 };
const color GREEN = { 0, 1, 0, 1 };
const color BLUE = { 0, 0, 1, 1 };
const color TRANSPARENT = { 0, 0, 0, 0 };

class image {
public:
    image(int height_, int width_) :
        height(height_),
        width(width_) {
    }

    void add_line(int y1, int x1, int y2, int x2, color stroke = BLACK) {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Element/line
        buf << "<line";
        buf << " y1=\"" << y1 << "\"";
        buf << " x1=\"" << x1 << "\"";
        buf << " y2=\"" << y2 << "\"";
        buf << " x2=\"" << x2 << "\"";
        write_color_spec("stroke", stroke);
        buf << "/>\n";
    }

    void add_path(const std::vector<std::pair<int, int> >& path, color stroke = BLACK, color fill = TRANSPARENT) {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Element/line
        buf << "<path";
        write_color_spec("stroke", stroke);
        write_color_spec("fill", fill);
        buf << " d=\"M";
        for (auto [y, x] : path) {
            buf << " " << x << "," << y;
        }
        buf << "\"/>\n";
    }

    void add_rect(int y, int x, int height, int width, color stroke = BLACK, color fill = TRANSPARENT) {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Element/rect
        buf << "<rect";
        buf << " y=\"" << y << "\"";
        buf << " x=\"" << x << "\"";
        buf << " height=\"" << height << "\"";
        buf << " width=\"" << width << "\"";
        write_color_spec("stroke", stroke);
        write_color_spec("fill", fill);
        buf << "/>\n";
    }

    void add_text(int y, int x, const std::string& text, color fill = BLACK, int size = 12, const std::string& text_anchor = "start") {
        // https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
        buf << "<text";
        buf << " y=\"" << y << "\"";
        buf << " x=\"" << x << "\"";
        buf << " font-size=\"" << size << "\"";
        if (text_anchor != "start") {
            buf << " text-anchor=\"" << text_anchor << "\"";
        }
        write_color_spec("fill", fill);
        buf << ">" << text << "</text>\n";
    }

private:
    template <class InputIterator> friend void write_movie(std::ostream& out, InputIterator first, InputIterator last);
    int height;
    int width;
    std::ostringstream buf;

    void write_color_spec(const std::string& name, const color& c) {
        int r = round(c.red * 255);
        int g = round(c.green * 255);
        int b = round(c.blue * 255);
        buf << " " << name << "=\"rgb(" << r << "," << g << "," << b << ")\"";
        if (c.alpha < 0.995) {
            buf << " " << name << "-opacity=\"" << std::fixed << std::setprecision(2) << c.alpha << "\"";
        }
    }
};

template <class InputIterator>
void write_movie(std::ostream& out, InputIterator first, InputIterator last) {
    out << R"HTML(<!DOCTYPE html>
<html><body>
<div id="text">loading...</div>
)HTML";
    int i = 0;
    for (; first != last; ++ first) {
        const image& frame = *first;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" id=\"f" << i << "\" viewBox=\"0 0 " << frame.width << " " << frame.height << "\" style=\"display: none;\">\n";
        out << frame.buf.str();
        out << "</svg>\n";
    }
    out << R"HTML(<script>
(function () {
    const text = document.getElementById("text");
    const svgs = document.getElementsByTagName("svg");
    let currentFrame = 0;
    let isPlaying = true;
    setInterval(() => {
        if (!isPlaying) return;
        text.innerText = (currentFrame + 1) + " / " + svgs.length;
        svgs[(currentFrame - 1 + svgs.length) % svgs.length].style.display = "none";
        svgs[currentFrame].style.display = null;
        currentFrame = (currentFrame + 1) % svgs.length;
        if (currentFrame == 0) {
            isPlaying = false;
        }
    }, 1000 / 60);
    window.onmousedown = function (e) {
       if (e.button == 0) {
           isPlaying = true;
       }
    };
})();
</script>
</body></html>
)HTML";
}

template <class InputIterator>
void write_movie_to_file(const std::string& path, InputIterator first, InputIterator last) {
    std::ofstream out(path);
    write_movie(out, first, last);
}

inline void write_movie_to_file(const std::string& path, const std::deque<image>& frames) {
    write_movie_to_file(path, frames.begin(), frames.end());
}

}  // namespace visualizer
