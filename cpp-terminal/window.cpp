#include "window.hpp"
#include <stdexcept>
#include "private/conversion.hpp"
#include "private/platform.hpp"

using namespace std;

/***************
   Term::Color
 ***************/

Term::Color::Color(const Term::fg val)
    : rgb_mode(false), r(0), g(0), fg_val(val)
{}

Term::Color::Color(const Term::bg val)
    : rgb_mode(false), r(0), g(0), bg_val(val)
{}

Term::Color::Color(const uint8_t a_r, const uint8_t a_g, const uint8_t a_b)
    : rgb_mode(true), r(a_r), g(a_g), b(a_b)
{}

bool Term::Color::is_rgb() const {return rgb_mode;}
uint8_t Term::Color::get_r() const {return r;}
uint8_t Term::Color::get_g() const {return g;}
uint8_t Term::Color::get_b() const {return b;}


/*****************
   Term::fgColor
 *****************/

Term::fgColor::fgColor() : Color(Term::fg::reset) {}
Term::fgColor::fgColor(fg c) : Color(c) {}
Term::fgColor::fgColor(const uint8_t a_r, const uint8_t a_g, const uint8_t a_b)
        : Color(a_r, a_g, a_b)
{}

Term::fg Term::fgColor::get_fg() const {return fg_val;}

std::string Term::fgColor::render() {
    if (rgb_mode) return Term::color24_fg(r, g, b);
    else return Term::color(fg_val);
}

bool Term::fgColor::is_reset() {
    return (rgb_mode == false && fg_val == fg::reset);
}

bool Term::operator==(const Term::fgColor &c1, const Term::fgColor &c2) {
    if (c1.is_rgb() != c2.is_rgb()) return false;
    if (c1.is_rgb()) return (c1.get_r() == c2.get_r()
                             && c1.get_g() == c2.get_g()
                             && c1.get_b() == c2.get_b());
    return (c1.get_fg() == c2.get_fg());
}

bool Term::operator!=(const Term::fgColor &c1, const Term::fgColor &c2) {
    return !(c1 == c2);
}



/*****************
   Term::bgColor
 *****************/

Term::bgColor::bgColor() : Color(Term::bg::reset) {}
Term::bgColor::bgColor(bg c) : Color(c) {}
Term::bgColor::bgColor(const uint8_t a_r, const uint8_t a_g, const uint8_t a_b)
    : Color(a_r, a_g, a_b)
{}
Term::bg Term::bgColor::get_bg() const {return bg_val;}

std::string Term::bgColor::render () {
    if (rgb_mode) return Term::color24_bg(r, g, b);
    else return Term::color(bg_val);
}

bool Term::bgColor::is_reset() {
    return (rgb_mode == false && bg_val == bg::reset);
}

bool Term::operator==(const Term::bgColor &c1, const Term::bgColor &c2) {
    if (c1.is_rgb() != c2.is_rgb()) return false;
    if (c1.is_rgb()) return (c1.get_r() == c2.get_r()
                             && c1.get_g() == c2.get_g()
                             && c1.get_b() == c2.get_b());
    return (c1.get_bg() == c2.get_bg());
}

bool Term::operator!=(const Term::bgColor &c1, const Term::bgColor &c2) {
    return !(c1 == c2);
}

/****************
 * Term::Window
 ****************
 */

Term::Window::Window(size_t w_, size_t h_)
    : w{w_}
    , h{h_}
    , fixed_width(true)
    , fixed_height(true)
    , cursor_visibility(true)
    , cursor_x{0}
    , cursor_y{0}
    , tabsize(0)
    , default_fg(fgColor(fg::reset))
    , default_bg(bgColor(bg::reset))
    , default_style(style::reset)
    , grid(h_)
{}

Term::Window::~Window() {
    for (ChildWindow* cwin : children) {
        delete cwin;
    }
}

void Term::Window::assure_pos(size_t x, size_t y){
    if (y >= h) {
        if (fixed_height) throw std::runtime_error("y out of bounds");
        h = y + 1;
        grid.resize(h);
    }
    if (x >= w) {
        if (fixed_width) throw std::runtime_error("x out of bounds");
        w = x + 1;
    }
    if (x >= grid[y].size()) {
        grid[y].resize(x + 1);
    }
}

Term::Window Term::Window::merge_children() const {
    Window res(w, h);
    res.grid = grid;
    res.cursor_x = cursor_x;
    res.cursor_y = cursor_y;
    for (const ChildWindow *cwin : children) {
        if (!cwin->is_visible()) continue;
        // in case there are nested children:
        // TODO is there a more elegant way? With less copying?
        Window mc = cwin->merge_children();
        size_t b = (cwin->border == border_type::NO_BORDER ? 0 : 1);
        // merge grid:
        for (size_t y = 0; y < cwin->h; ++y) {
            size_t pos_y = y + cwin->offset_y;
            if (pos_y >= h) break;
            for (size_t x = 0; x < cwin->w; ++x) {
                size_t pos_x = x + cwin->offset_x;
                if (pos_x >= w) break;
                res.set_cell(pos_x, pos_y, mc.get_cell(x, y));
            }
        }
        // draw border:
        if (cwin->border == border_type::NO_BORDER) {
            continue;
        }  
        // 
        res.print_rect(  
            // TODO include title right away?!
            cwin->offset_x ? cwin->offset_x - 1 : 0,
            cwin->offset_y ? cwin->offset_y - 1 : 0,
            cwin->offset_x ? cwin->w + 2 : cwin->w + 1,
            cwin->offset_y ? cwin->h + 2 : cwin->h + 1,
            cwin->border,
            cwin->border_fg,
            cwin->border_bg
        );
        // print title
        if (!cwin->title.size()) {
            continue;
        }
        // title out of window?
        if (cwin->offset_y > h) {
            continue;
        }
        u32string title_cp = cwin->title.substr(0, cwin->w);
        if (title_cp.size() <= cwin->w - 2) {
            title_cp = U" " + title_cp + U" ";
        }           
        size_t x0 = cwin->offset_x + (cwin->w - title_cp.size()) / 2;
        for (size_t i = 0; i != title_cp.size() && x0 + i < w; ++i) {
            // color has already been set to border_fg/border_bg
            res.set_char(x0 + i, cwin->offset_y - 1, title_cp[i]);
        }
    }
    return res;
}

char32_t Term::Window::get_char(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x].ch;
    else return U' ';
}

Term::fgColor Term::Window::get_fg(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x].cell_fg;
    else return default_fg;
}

Term::bgColor Term::Window::get_bg(size_t x, size_t y) const {
   if (y < grid.size() && x < grid[y].size())
        return grid[y][x].cell_bg;
    else return default_bg;
}

Term::style Term::Window::get_style(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x].cell_style;
    else return default_style;
}

Term::Cell Term::Window::get_cell(size_t x, size_t y) const {
    if (y < grid.size() && x < grid[y].size())
        return grid[y][x];
    else return Cell(U' ', default_fg, default_bg, default_style);
}

void Term::Window::set_cell(size_t x, size_t y, const Term::Cell &c) {
    assure_pos(x, y);
    grid[y][x] = c;
}


vector<vector<Term::Cell>> Term::Window::get_grid() const {
    return grid;
}

void Term::Window::set_grid(const vector<vector<Term::Cell>> &new_grid) {
    grid = new_grid;
    if (grid.size() > h) {
        if (fixed_height) grid.resize(h);
        else h = grid.size();
    }
    for (vector<Cell> &row : grid) {
        if (row.size() > w) {
            if (fixed_width) row.resize(w);
            else w = row.size();
        }
    }
}

void Term::Window::copy_grid_from(const Term::Window & win) {
    set_grid(win.get_grid());
}

void Term::Window::clear_grid() {
    grid.clear();
    grid.resize(h);
    cursor_x = 0;
    cursor_y = 0;
}

size_t Term::Window::get_w() const {
    return w;
}

void Term::Window::set_w(size_t new_w) {
    if (new_w == w) return;
    w = new_w;
    for (size_t y = 0; y != grid.size(); ++y) {
        if (grid[y].size() > w) grid[y].resize(w);
    }
}

void Term::Window::trim_w(size_t minimal_width) {
    if (w <= minimal_width) {
        set_w(minimal_width);
        return;
    }
    size_t x = minimal_width;
    for (const auto &row : grid) x = max(x, row.size());
    // make sure the cursor remains in the window
    x = max(x, cursor_x + 1);
    set_w(x);
}

size_t Term::Window::get_h() const {
    return h;
}

void Term::Window::set_h(size_t new_h) {
    grid.resize(new_h);
    h = new_h;
}

void Term::Window::trim_h(size_t minimal_height) {
    if (h <= minimal_height || grid.size() <= minimal_height) {
        set_h(minimal_height);
        return;
    }
    size_t y = min(h, grid.size());
    for(; y > minimal_height; --y) {
        if (grid[y - 1].size()) break;
    }
    // make sure the cursor remains in the window
    y = max(y, cursor_y + 1);
    set_h(y);
    // TODO take non-hidden sub-windows into account
}

void Term::Window::resize(size_t new_w, size_t new_h) {
    set_w(new_w);
    set_h(new_h);
}

void Term::Window::trim(size_t min_w, size_t min_h) {
    trim_h(min_h);
    trim_w(min_w);
}

bool Term::Window::is_fixed_width() const {
    return fixed_width;
}

void Term::Window::set_fixed_width(bool a) {
    fixed_width = a;
}

bool Term::Window::is_fixed_height() const {
    return fixed_height;
}

void Term::Window::set_fixed_height(bool a) {
    fixed_height = a;
}

size_t Term::Window::get_cursor_x() const {
    return cursor_x;
}

size_t Term::Window::get_cursor_y() const {
    return cursor_y;
}

bool Term::Window::is_cursor_visible() const {
    return cursor_visibility;
}

void Term::Window::show_cursor() {
    cursor_visibility = true;
}

void Term::Window::hide_cursor() {
    cursor_visibility = false;
}



void Term::Window::set_char(size_t x, size_t y, char32_t c) {
    assure_pos(x, y);
    grid[y][x].ch = c;
}




void Term::Window::set_default_fg(fgColor c) {
    default_fg = c;
}

void Term::Window::set_default_fg(uint8_t r, uint8_t g, uint8_t b) {
    default_fg = fgColor(r, g, b);
}

void Term::Window::set_fg(size_t x, size_t y, fgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_fg = c;
}

void Term::Window::set_fg(size_t x, size_t y,
                          uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_fg = fgColor(r, g, b);
}

void Term::Window::set_default_bg(bgColor c) {
    default_bg = c;
}

void Term::Window::set_default_bg(uint8_t r, uint8_t g, uint8_t b) {
    default_bg = bgColor(r, g, b);
}

void Term::Window::set_bg(size_t x, size_t y, bgColor c) {
    assure_pos(x, y);
    grid[y][x].cell_bg = c;
}

void Term::Window::set_bg(size_t x, size_t y,
                          uint8_t r, uint8_t g, uint8_t b) {
    assure_pos(x, y);
    grid[y][x].cell_bg = bgColor(r, g, b);
}

void Term::Window::set_default_style(style c) {
    default_style = c;
}

void Term::Window::set_style(size_t x, size_t y, style c) {
    assure_pos(x, y);
    grid[y][x].cell_style = c;
}

void Term::Window::set_cursor(size_t x, size_t y) {
    assure_pos(x, y);
    cursor_x = x;
    cursor_y = y;
}

size_t Term::Window::get_tabsize() const {
    return tabsize;
}

void Term::Window::set_tabsize(size_t ts) {
    tabsize = ts;
}


size_t Term::Window::print_str(const std::u32string& s,
                   fgColor a_fg, bgColor a_bg, style a_style)
{
    using Term::Key;
    if (a_fg == fg::unspecified) a_fg = default_fg;
    if (a_bg == bg::unspecified) a_bg = default_bg;
    if (a_style == style::unspecified) a_style = default_style;
    if (cursor_y >= h && fixed_height) {
        // out of the window. Should not happen here: just to be safe.
        cursor_y = h - 1;
        if (cursor_x >= w) cursor_x = w - 1;
        return 0;
    }
    size_t x = cursor_x;
    size_t y = cursor_y;

    size_t i = 0;
    for (; i != s.size(); ++i) {
        char32_t ch = s[i];
        bool newline = (ch == ENTER || (x >= w && fixed_width));
        if (newline) {
            ++y;
            if (y >= h) {
                if (fixed_height) {
                    // out of the window
                    y = h - 1;
                    if (x >= w) x = w - 1;
                    break;
                }
                // adjust window height
                set_h(y + 1);
            }
            x = 0;
            if (ch == ENTER) continue;
        }

        // New
        // TODO: tab?
        if (ch >= U' ' && ch <= UTF8_MAX) {
            if ((x >= w && fixed_width) || (y >= h && fixed_height)) {
                // out of the window
                break;
            }
            set_char(x, y, ch);
            set_fg(x, y, a_fg);
            set_bg(x, y, a_bg);
            set_style(x, y, a_style);
            ++x;
            if (x < w) continue;
            // Right margin exceeded. Ignore that if next character in
            // the string is '\n' anyway:
            if (i + 1 != s.size() && s[i + 1] == ENTER) continue;
            // Unless fixed, adjust the width of the window
            if (!fixed_width) {
                set_w (x + 1);
                continue;
            }
            // Otherwise, begin new line ...
            if (y < h - 1) {
                ++y;
                x = 0;
                continue;
            }
            // ... or, if at bottom line, either grow height if allowed
            if (!fixed_height) {
                ++y;
                set_h(y + 1);
                x = 0;
                continue;
            }
            // ... or keep cursor in bottom right corner and exit loop
            y = h - 1;
            x = w - 1;
            // before break, count the character
            ++i;
            break;
        }
    }
    // set cursor
    cursor_x = x;
    cursor_y = y;
    return i;
}

size_t Term::Window::print_str(const std::string& s,
                   fgColor a_fg, bgColor a_bg, style a_style)
{
    std::u32string s32 = Private::utf8_to_utf32(s);
    return print_str(s32, a_fg, a_bg, a_style);
}


void Term::Window::fill_fg(size_t x1, size_t y1,
                           size_t width, size_t height, fgColor color) {
    if (color == fg::unspecified) color = default_fg;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_fg(i, j, color);
        }
    }
}

void Term::Window::fill_bg(size_t x1, size_t y1,
                           size_t width, size_t height, bgColor color) {
    if (color == bg::unspecified) color = default_bg;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_bg(i, j, color);
        }
    }
}

void Term::Window::fill_style(size_t x1, size_t y1,
                              size_t width, size_t height, style st) {
    if (st == style::unspecified) st = default_style;
    for (size_t j = y1; j < y1 + height; j++) {
        for (size_t i = x1; i < x1 + width; i++) {
            set_style(i, j, st);
        }
    }
}

void Term::Window::print_rect(
    size_t x1,
    size_t y1,
    size_t width,
    size_t height,
    border_type b,
    fgColor fgcol,
    bgColor bgcol
)
// A rectangle does not auto-grow the window.
// Only the in-window parts are drawn.
{
    if (x1 >= w || y1 >= h) return;
    std::u32string border;
    switch (b) {
    case border_type::NO_BORDER : return;
    case border_type::BLANK : border = U"      "; break;
    case border_type::ASCII : border = U"|-++++"; break;
    case border_type::LINE : border = U"│─┌┐└┘"; break;
    case border_type::DOUBLE_LINE : border = U"║═╔╗╚╝"; break;
    default: throw runtime_error ("undefined border value");
    }
    size_t x2 = x1 + width - 1, y2 = y1 + height - 1;
    for (size_t y = y1 + 1; y < y2 && y < h; ++y) {
        set_char(x1, y, border[0]);
        set_fg(x1, y, fgcol);
        set_bg(x1, y, bgcol);
        if (x2 < w) {
            set_char(x2, y, border[0]);
            set_fg(x2, y, fgcol);
            set_bg(x2, y, bgcol);
        }
    }
    for (size_t x = x1 + 1; x < x2 && x < w; ++x) {
        set_char(x, y1, border[1]);
        set_fg(x, y1, fgcol);
        set_bg(x, y1, bgcol);
        if (y2 < h) {
            set_char(x, y2, border[1]);
            set_fg(x, y2, fgcol);
            set_bg(x, y2, bgcol);
        }
    }
    set_char(x1, y1, border[2]);
    set_fg(x1, y1, fgcol);
    set_bg(x1, y1, bgcol);
    if (x2 < w) {
        set_char(x2, y1, border[3]);
        set_fg(x2, y1, fgcol);
        set_bg(x2, y1, bgcol);
    }
    if (y2 < h) {
        set_char(x1, y2, border[4]);
        set_fg(x1, y2, fgcol);
        set_bg(x1, y2, bgcol);
        if (x2 < w) {
            set_char(x2, y2, border[5]);
            set_fg(x2, y2, fgcol);
            set_bg(x2, y2, bgcol);
        }
    }
}

void Term::Window::clear() {
    clear_grid();
    children.clear();
}

void Term::Window::clear_row(size_t y) {
    if (y < grid.size()) {
        grid[y].clear();
    }
}

string Term::Window::render(size_t x0, size_t y0,
                                 size_t width, size_t height) {
    // TODO move this into Terminal::draw_window()?
    if (children.size()) return merge_children().render(x0, y0, width, height);

    string out;// = cursor_off() + clear_screen() + move_cursor(0, 0);
    fgColor current_fg(fg::reset);
    bgColor current_bg(bg::reset);
    style current_style = style::reset;
    const size_t x1 = (width == string::npos ? w : min(w, x0 + width));
    const size_t y1 = (height == string::npos ? h : min(h, y0 + height));
    for (size_t j = y0; j < y1; j++) {
        if (j > y0) {
            // Resetting background color at the end of each line
            // is a workaround for the bug in Visual Studio Code
            // (https://github.com/jupyter-xeus/cpp-terminal/issues/95)
            if (!current_bg.is_reset()) {
                out.append(color(bg::reset));
                current_bg = bg::reset;
            }
            out.append("\n");
        }
        size_t i = x0;
        for (; i < x1; i++) {
            bool update_fg = false;
            bool update_bg = false;
            bool update_style = false;
            Cell current_cell = get_cell(i, j);
            if (current_fg != current_cell.cell_fg) {
                current_fg = current_cell.cell_fg;
                update_fg = true;
            }
            if (current_bg != current_cell.cell_bg) {
                current_bg = current_cell.cell_bg;
                update_bg = true;
            }
            if (current_style != current_cell.cell_style) {
                current_style = current_cell.cell_style;
                update_style = true;
                if (current_style == style::reset) {
                    // style::reset resets fg and bg colors too, we have to
                    // set them again if they are non-default, but if fg or
                    // bg colors are reset, we do not update them, as
                    // style::reset already did that.
                    update_fg = !current_fg.is_reset();
                    update_bg = !current_bg.is_reset();
                }
            }
            // Set style first, as style::reset will reset colors too
            if (update_style)
                out.append(color(current_cell.cell_style));
            if (update_fg)
                out.append(current_cell.cell_fg.render());
            if (update_bg)
                out.append(current_cell.cell_bg.render());
            Private::codepoint_to_utf8(out, current_cell.ch);
        }
        //if (i < w) out.append(erase_to_eol());
    }
    // reset colors and style at the end
    if (!current_fg.is_reset())
        out.append(color(fg::reset));
    if (!current_bg.is_reset())
        out.append(color(bg::reset));
    if (current_style != style::reset)
    out.append(color(style::reset));
    //out.append(move_cursor(cursor_x, cursor_y));
    //if (cursor_visibility) out.append(cursor_on());
    //else out.append(cursor_off()); //necessary?
    return out;
}

Term::Window Term::Window::cutout(size_t x0, size_t y0,
                                  size_t width, size_t height) const {
    Window cropped(width, height);
    for (size_t y = y0; y != y0 + height && y < h; ++y) {
        for (size_t x = x0; x != x0 + width && x < grid[y].size(); ++x) {
            cropped.grid[y].push_back(grid[y][x]);
        }
    }
    // preserve cursor if within cut-out
    if (cursor_x < x0 || cursor_x >= x0 + width || cursor_y < y0
        || cursor_y >= y0 + height) {
        cropped.set_cursor(0, 0);
    }
    else {
        cropped.set_cursor(cursor_x - x0, cursor_y - y0);
    }
    return cropped;
}

Term::ChildWindow* Term::Window::new_child(size_t o_x, size_t o_y,
                                    size_t w_, size_t h_, border_type b) {
    ChildWindow* cwin = new ChildWindow(this, o_x, o_y, w_, h_, b);
    children.push_back(cwin);
    return cwin;
}

Term::ChildWindow* Term::Window::get_child_ptr(size_t i) {
    if (i < children.size()) return children[i];
    throw runtime_error("Term::Window::get_child(): "
                        "child index out of bounds");
}

size_t Term::Window::get_child_index(ChildWindow *cwin) const {
    for (size_t i = 0; i != children.size(); ++i) {
        if (children[i] == cwin)
            return i;
    }
    throw runtime_error("get_child_index(): argument is not a child of *this");
}

size_t Term::Window::get_children_count() const {
    return children.size();
}

void Term::Window::child_to_foreground(ChildWindow* cwin) {
    size_t i = get_child_index(cwin);
    size_t j = children.size() - 1;
    if (i == j)
        return;
    ChildWindow* temp = children[j];
    children[j] = children[i];
    children[i] = temp;
}


void Term::Window::child_to_background(ChildWindow* cwin) {
    size_t i = get_child_index(cwin);
    if (i == 0)
        return;
    ChildWindow* temp = children[0];
    children[0] = children[i];
    children[i] = temp;
}


void Term::Window::take_cursor_from_child(ChildWindow* cwin) {
    if (cwin == this)
        return;
    if (cwin->is_base_window())
        throw runtime_error(
            "take_cursor_from_child(): argument is not a child");
    if (!cwin->is_visible() || !cwin->cursor_visibility) {
        cursor_visibility = false;
        return;
    }
    Window* pwin = cwin->get_parent_ptr();
    size_t x = cwin->cursor_x;
    size_t y = cwin->cursor_y;
    // Cursor obscured by child's own child?
    for (ChildWindow* cwin2 : cwin->children) {
        if (!cwin2->visible) {
            continue;
        }
        size_t b = (cwin2->border == border_type::NO_BORDER ? 0 : 1);
        if (x + b >= cwin2->offset_x && x < cwin2->offset_x + cwin2->w + b &&
            y + b >= cwin2->offset_y && y < cwin2->offset_y + cwin2->h + b) {
            cursor_visibility = false;
            return;
        }
    }
    x += cwin->offset_x;
    y += cwin->offset_y;
    if (x >= pwin->w || y >= pwin->h) {
        cursor_visibility = false;
        return;
    }
    // pass cursor on to base window
    while (!pwin->is_base_window()) {
        cwin = dynamic_cast<ChildWindow*>(pwin);
        pwin = cwin->get_parent_ptr();
        x += cwin->offset_x;
        y += cwin->offset_y;
        if (!cwin->is_visible() || x >= pwin->w || y >= pwin->h) {
            cursor_visibility = false;
            return;
        }
    }
    if (pwin != this)
        throw runtime_error(
            "take_cursor_from_child(): argument does not belong to *this"); 
    cursor_x = x;
    cursor_y = y;
    // is cursor obscured by another window?
    bool is_more_to_the_fore = false;
    for (ChildWindow *cwin2 : children) {
        if (cwin2 == cwin) {
            is_more_to_the_fore = true;
            continue;
        }
        if (!is_more_to_the_fore || !cwin2->visible) {
            continue;
        }
        size_t b = (cwin2->border == border_type::NO_BORDER ? 0 : 1);
        if (x + b >= cwin2->offset_x &&
            x < cwin2->offset_x + cwin2->w + b &&
            y + b >= cwin2->offset_y &&
            y < cwin2->offset_y + cwin2->h + b) {
            cursor_visibility = false;
            return;
        }
    }
    cursor_visibility = true;
    return;
}



/*********************
 * Term::ChildWindow
 *********************
 */

Term::ChildWindow::ChildWindow(Window* ptr,
                               size_t off_x, size_t off_y,
                               size_t w_, size_t h_, border_type b)
    : parent_ptr(ptr)
    , Window(w_, h_)
    , offset_x(off_x)
    , offset_y(off_y)
    , border(b)
    , visible(false)
{}

Term::Window* Term::ChildWindow::get_parent_ptr() const {
    return parent_ptr;
}

size_t Term::ChildWindow::get_offset_x() const {
    return offset_x;
}

size_t Term::ChildWindow::get_offset_y() const {
    return offset_y;
}

pair<size_t, size_t> Term::ChildWindow::move_to(size_t x, size_t y) {
    size_t border_width = (border == border_type::NO_BORDER ? 0 : 1);
    if (x < border_width) {
        offset_x = border_width;
    } else {
        size_t max_x = parent_ptr->get_w() - w - border_width;
        offset_x = min(x, max_x);
    }
    if (y < border_width) {
        offset_y = border_width;
    } else {
        size_t max_y = parent_ptr->get_h() - h - border_width;
        offset_y = min(y, max_y);
    }
    return make_pair(offset_x, offset_y);
}

std::u32string Term::ChildWindow::get_title() const {
    return title;
}

void Term::ChildWindow::set_title(const std::string& s) {
    set_title(Term::Private::utf8_to_utf32(s));
}

void Term::ChildWindow::set_title(const std::u32string& s) {
    title = s;
}

Term::border_type Term::ChildWindow::get_border() const {
    return border;
}

void Term::ChildWindow::set_border(Term::border_type b,
                                   fgColor fgcol, bgColor bgcol) {
    border = b;
    border_fg = fgcol;
    border_bg = bgcol;
}

Term::fgColor Term::ChildWindow::get_border_fg() const {
    return border_fg;
}

void Term::ChildWindow::set_border_fg(fgColor fgcol) {
    border_fg = fgcol;
}

Term::bgColor Term::ChildWindow::get_border_bg() const {
    return border_bg;
}

void Term::ChildWindow::set_border_bg(bgColor bgcol) {
    border_bg = bgcol;
}

bool Term::ChildWindow::is_visible() const {
    return visible;
}

void Term::ChildWindow::show() {
    visible = true;
}

void Term::ChildWindow::hide() {
    visible = false;
}



