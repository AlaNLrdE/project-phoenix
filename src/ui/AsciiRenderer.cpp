#include <ui/AsciiRenderer.hpp>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Phoenix::UI
{

    // ─── Constructor / clear ──────────────────────────────────────────────────────

    AsciiRenderer::AsciiRenderer(int width, int height)
        : width_(width), height_(height),
          canvas_(height, std::vector<char>(width, ' ')),
          xmin_(-1.0), xmax_(1.0), ymin_(-1.0), ymax_(1.0)
    {
    }

    void AsciiRenderer::clear()
    {
        for (auto &row : canvas_)
            std::fill(row.begin(), row.end(), ' ');
    }

    void AsciiRenderer::putChar(int x, int y, char c)
    {
        if (x >= 0 && x < width_ && y >= 0 && y < height_)
            canvas_[y][x] = c;
    }

    void AsciiRenderer::setViewport(double xmin, double xmax, double ymin, double ymax)
    {
        xmin_ = xmin;
        xmax_ = xmax;
        ymin_ = ymin;
        ymax_ = ymax;
    }

    // ─── Coordinate conversion ────────────────────────────────────────────────────

    std::pair<int, int> AsciiRenderer::worldToCanvas(double wx, double wy) const
    {
        double tx = (xmax_ > xmin_) ? (wx - xmin_) / (xmax_ - xmin_) : 0.5;
        double ty = (ymax_ > ymin_) ? (wy - ymin_) / (ymax_ - ymin_) : 0.5;
        int cx = static_cast<int>(tx * (width_ - 1) + 0.5);
        int cy = static_cast<int>((1.0 - ty) * (height_ - 1) + 0.5); // Y invertido
        return {cx, cy};
    }

    // ─── Drawing primitives ───────────────────────────────────────────────────────

    void AsciiRenderer::drawBody(double worldRadius, char c)
    {
        // Sample step: aim for ~0.6 canvas pixels to ensure full fill
        double dx = (xmax_ - xmin_) / width_;
        double dy = (ymax_ - ymin_) / height_;
        double step = std::min(dx, dy) * 0.6;
        step = std::max(step, worldRadius / 100.0); // avoid too many iterations

        for (double x = -worldRadius; x <= worldRadius; x += step)
        {
            for (double y = -worldRadius; y <= worldRadius; y += step)
            {
                if (x * x + y * y <= worldRadius * worldRadius)
                {
                    auto [cx, cy] = worldToCanvas(x, y);
                    putChar(cx, cy, c);
                }
            }
        }
    }

    void AsciiRenderer::drawOrbit(const Orbit &orbit, char c, int samples)
    {
        double e = orbit.e;
        double a = orbit.a;
        double p = a * (1.0 - e * e); // semi-latus rectum

        for (int i = 0; i < samples; ++i)
        {
            double nu = (2.0 * M_PI * i) / samples;
            double r = p / (1.0 + e * std::cos(nu));
            double wx = r * std::cos(nu); // perifocal X (periapsis direction)
            double wy = r * std::sin(nu); // perifocal Y
            auto [cx, cy] = worldToCanvas(wx, wy);
            putChar(cx, cy, c);
        }
    }

    void AsciiRenderer::drawAxes()
    {
        // Horizontal axis (world Y = 0) — only if Y=0 is in viewport
        if (ymin_ <= 0.0 && ymax_ >= 0.0)
        {
            auto [x0, yax] = worldToCanvas(xmin_, 0.0);
            auto [x1, dummy] = worldToCanvas(xmax_, 0.0);
            for (int x = x0; x <= x1; ++x)
                putChar(x, yax, '-');
        }

        // Vertical axis (world X = 0) — only if X=0 is in viewport
        if (xmin_ <= 0.0 && xmax_ >= 0.0)
        {
            auto [xax, y0] = worldToCanvas(0.0, ymin_);
            auto [dummy, y1] = worldToCanvas(0.0, ymax_);
            int ylo = std::min(y0, y1);
            int yhi = std::max(y0, y1);
            for (int y = ylo; y <= yhi; ++y)
                putChar(xax, y, ':');
        }

        // Intersection
        if (xmin_ <= 0.0 && xmax_ >= 0.0 && ymin_ <= 0.0 && ymax_ >= 0.0)
        {
            auto [xax, yax] = worldToCanvas(0.0, 0.0);
            putChar(xax, yax, '+');
        }
    }

    // ─── Render ───────────────────────────────────────────────────────────────────

    void AsciiRenderer::render(std::ostream &os, const std::string &title) const
    {
        // Top border with optional title
        std::string top(width_, '-');
        if (!title.empty() && static_cast<int>(title.size()) + 4 <= width_)
        {
            int pos = (width_ - static_cast<int>(title.size()) - 2) / 2;
            top.replace(pos, 1, " ");
            top.replace(pos + 1, title.size(), title);
            top.replace(pos + 1 + static_cast<int>(title.size()), 1, " ");
        }

        os << "+" << top << "+\n";
        for (const auto &row : canvas_)
        {
            os << '|';
            for (char ch : row)
                os << ch;
            os << "|\n";
        }
        os << "+" << std::string(width_, '-') << "+\n";
    }

    // ─── Static convenience methods ───────────────────────────────────────────────

    void AsciiRenderer::displayOrbit(const Orbit &orbit, double bodyRadius,
                                     const std::string &title)
    {
        AsciiRenderer r(78, 22);

        double a = orbit.a;
        double e = orbit.e;
        double rp = a * (1.0 - e);                            // periapsis distance
        double ra = a * (1.0 + e);                            // apoapsis distance
        double b = a * std::sqrt(std::max(0.0, 1.0 - e * e)); // semi-minor axis

        // Center viewport on the geometric center of the ellipse (-a*e, 0).
        // Stretch X by ~2× to compensate for monospace char aspect ratio (~2:1 h:w).
        double pad = ra * 0.08;
        double yHalf = b + pad;
        double xHalf = yHalf * (78.0 / 22.0) * 0.5; // canvas aspect × char aspect (0.5)
        // Ensure full orbit fits
        xHalf = std::max(xHalf, ra + pad);
        double cx = -a * e; // geometric centre offset from focus

        r.setViewport(cx - xHalf, cx + xHalf, -yHalf, yHalf);
        r.clear();
        r.drawAxes();
        r.drawOrbit(orbit, '*');
        r.drawBody(bodyRadius, 'O');

        // Periapsis (P) and apoapsis (A) markers
        {
            auto [px, py] = r.worldToCanvas(rp, 0.0);
            auto [ax, ay] = r.worldToCanvas(-ra, 0.0);
            r.putChar(px, std::max(0, py - 1), 'P');
            r.putChar(ax, std::max(0, ay - 1), 'A');
        }

        r.render(std::cout, title);

        std::cout << "  P=Periapsis(" << std::fixed << std::setprecision(0)
                  << rp / 1000.0 << " km)  A=Apoapsis(" << ra / 1000.0
                  << " km)  *=Órbita  O=Cuerpo\n";
    }

    void AsciiRenderer::displayReentryProfile(const AeroTrajectoryResult &result,
                                              const std::string &title)
    {
        if (result.points.empty())
            return;

        double tMax = result.points.back().time;
        double altMax = 0.0;
        double vMax = 0.0;
        for (const auto &p : result.points)
        {
            altMax = std::max(altMax, p.altitude);
            vMax = std::max(vMax, p.speed);
        }
        if (altMax <= 0.0 || tMax <= 0.0)
            return;

        AsciiRenderer r(78, 16);
        r.setViewport(0.0, tMax, 0.0, altMax * 1.08);
        r.clear();

        // Ground line
        {
            auto [x0, gy] = r.worldToCanvas(0.0, 0.0);
            for (int x = 0; x < 78; ++x)
                r.putChar(x, gy, '_');
        }

        // Altitude trace (*)
        for (const auto &p : result.points)
        {
            auto [cx, cy] = r.worldToCanvas(p.time, p.altitude);
            r.putChar(cx, cy, '*');
        }

        // Speed trace normalised to altitude scale (:)
        for (const auto &p : result.points)
        {
            double normV = (vMax > 0.0) ? (p.speed / vMax) * altMax : 0.0;
            auto [cx, cy] = r.worldToCanvas(p.time, normV);
            r.putChar(cx, cy, ':');
        }

        r.render(std::cout, title);

        std::cout << std::fixed << std::setprecision(1)
                  << "  * = Altitud (0–" << altMax / 1000.0 << " km)"
                  << "   : = Velocidad (0–" << vMax / 1000.0 << " km/s)"
                  << "   Duración: " << tMax << " s\n";
    }

} // namespace Phoenix::UI
