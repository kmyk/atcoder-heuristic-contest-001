// https://gist.github.com/saharan/9547f3e56bcfb3e355a9d1df4d670fd4
// 末尾に使用例
// 好きに改造して使ってください

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// using namespace std;

class Graphics {
public:
	double screenW;
	double screenH;
	std::ostringstream data;

	double sr;
	double sg;
	double sb;
	double sa;
	double fr;
	double fg;
	double fb;
	double fa;

	Graphics() : screenW(1), screenH(1), sr(0), sg(0), sb(0), sa(1), fr(1), fg(1), fb(1), fa(1) {
	}

	void screen(int width, int height) {
		screenW = width;
		screenH = height;
	}

	void clear() {
		data.str("");
		data.clear(std::stringstream::goodbit);
	}

	void stroke(double r, double g, double b) {
		stroke(r, g, b, 1);
	}

	void stroke(double r, double g, double b, double a) {
		sr = r;
		sg = g;
		sb = b;
		sa = a;
	}

	void noStroke() {
		stroke(0, 0, 0, 0);
	}

	void fill(double r, double g, double b) {
		fill(r, g, b, 1);
	}

	void fill(double r, double g, double b, double a) {
		fr = r;
		fg = g;
		fb = b;
		fa = a;
	}

	void noFill() {
		fill(0, 0, 0, 0);
	}

	void line(double x1, double y1, double x2, double y2) {
		data << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\" " << stroke() << "/>\n";
	}

	void rect(double x, double y, double w, double h) {
		data << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"" << h << "\" " << stroke() << " " + fill() << "/>\n";
	}

	void text(std::string str, double x, double y, double size = 16) {
		data << "<text text-anchor=\"middle\" x=\"" << x << "\" y=\"" << y << "\" font-size=\"" << size << "\" " << fill() << " >" << str << "</text>\n";
	}

	std::string dump(std::string id = "", std::string style = "", int widthPx = -1, int heightPx = -1) const {
		std::ostringstream res;
		res << "<svg ";
		if (id != "") res << "id=\"" + id + "\" ";
		if (style != "") res << "style=\"" + style + "\" ";
		if (widthPx != -1) res << "width=\"" << widthPx << "\" ";
		if (heightPx != -1) res << "height=\"" << heightPx << "\" ";
		res << "viewBox=\"-1 -1 " << screenW + 2 << " " << screenH + 2 << "\" xmlns=\"http://www.w3.org/2000/svg\">\n" << data.str() << "</svg>";
		return res.str();
	}

private:
	std::string stroke() const {
		return "stroke=\"" + rgb(sr, sg, sb) + "\" stroke-opacity=\"" + s(sa) + "\"";
	}

	std::string fill() const {
		return "fill=\"" + rgb(fr, fg, fb) + "\" fill-opacity=\"" + s(fa) + "\"";
	}

	std::string rgb(double r, double g, double b) const {
		return "rgb(" + s(lround(r * 255)) + "," + s(lround(g * 255)) + "," + s(lround(b * 255)) + ")";
	}

	std::string s(double x) const {
		return std::to_string(x);
	}
};

class Movie {
public:
	std::vector<std::string> svgs;

	Movie() {
	}

	void clear() {
		svgs.clear();
	}

	void addFrame(Graphics& g) {
		svgs.push_back(g.dump("f" + std::to_string(svgs.size()), "display:none;pointer-events:none;user-select:none;"));
	}

	std::string dumpHtml(int fps) {
		std::ostringstream res;
		res << "<html><body><div id=\"text\">loading...</div>" << std::endl;
		for (std::string& svg : svgs) {
			res << svg << std::endl;
		}

		res << "<script>\nlet numFrames = " << svgs.size() << ", fps = " << fps << ";";
		res << R"(
	let text = document.getElementById("text");
	let frames = [];
	for (let i = 0; i < numFrames; i++) {
		let f = document.getElementById("f" + i);
		frames.push(f);
		f.style.display = "none";
	}
	let currentFrame = 0;
	let playing = true;
	setInterval(() => {
		if (!playing) return;
		text.innerText = (currentFrame + 1) + " / " + numFrames;
		frames[(currentFrame - 1 + numFrames) % numFrames].style.display = "none";
		frames[currentFrame].style.display = null;
		currentFrame = (currentFrame + 1) % numFrames;
		if (currentFrame == 0) playing = false;
	}, 1000 / fps);
	window.onmousedown = e => { if (e.button == 0) playing = true; };
;)";
		res << "</script>" << std::endl;
		res << "</body></html>" << std::endl;
		return res.str();
	}
private:
};

/*
int main() {
	Graphics g;
	Movie mov;

	int w = 500;
	int h = 300;
	double x = 100;
	double y = 100;
	double vx = 20;
	double vy = 25;

	// スクリーンの大きさを設定
	g.screen(w, h);

	for (int i = 0; i < 60 * 5; i++) {
		// 画面消去
		g.clear();
		// 枠を黒に、塗りを灰色に
		g.stroke(0, 0, 0);
		g.fill(0.5, 0.5, 0.5);
		// 四角を描画
		g.rect(x - 50, y - 50, 100, 100);
		// 座標を青で表示
		g.fill(0, 0, 1);
		g.text("X=" + std::to_string(x) + ", Y=" + std::to_string(y), w / 2, 100);
		// フレームを追加
		mov.addFrame(g);
		// 四角を動かす
		x += vx;
		y += vy;
		// 重力
		vy += 2;
		// 壁で跳ね返す
		if (x < 50 && vx < 0 || x > w - 50 && vx > 0) vx = -vx * 0.8;
		if (y < 50 && vy < 0 || y > h - 50 && vy > 0) vy = -vy * 0.8;
	}
	// 60FPS で書き出し
	std::string html = mov.dumpHtml(60);

	// 動画を保存
	std::ofstream fout;
	fout.open("movie.html", std::ios::out);
	if (!fout) {
		return 1;
	}
	fout << html << std::endl;
	fout.close();

	return 0;
}
*/
