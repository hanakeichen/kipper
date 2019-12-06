#pragma once

#include <algorithm>
#include <string_view>

namespace kipper {
namespace internal {

class Position {
 public:
  explicit Position(std::string_view f = std::string_view{}, unsigned l = 1u,
                    unsigned c = 1u)
      : filename{f}, line(l), column(c) {}

  void initialize(std::string_view f = std::string_view{}, unsigned l = 1u,
                  unsigned c = 1u) {
    filename = f;
    line = l;
    column = c;
  }

  void lines(int count = 1) {
    if (count) {
      column = 1u;
      line = add_(line, count, 1);
    }
  }

  void columns(int count = 1) { column = add_(column, count, 1); }

  /// File name to which this position refers.
  std::string_view filename;
  /// Current line number.
  unsigned line;
  /// Current column number.
  unsigned column;

 private:
  /// Compute max (min, lhs+rhs).
  static unsigned add_(unsigned lhs, int rhs, int min) {
    return static_cast<unsigned>(std::max(min, static_cast<int>(lhs) + rhs));
  }
};

/// Add \a width columns, in place.
inline Position& operator+=(Position& res, int width) {
  res.columns(width);
  return res;
}

/// Add \a width columns.
inline Position operator+(Position res, int width) { return res += width; }

/// Subtract \a width columns, in place.
inline Position& operator-=(Position& res, int width) { return res += -width; }

/// Subtract \a width columns.
inline Position operator-(Position res, int width) { return res -= width; }

/// Compare two position objects.
inline bool operator==(const Position& pos1, const Position& pos2) {
  return pos1.filename == pos2.filename && pos1.line == pos2.line &&
         pos1.column == pos2.column;
}

/// Compare two position objects.
inline bool operator!=(const Position& pos1, const Position& pos2) {
  return !(pos1 == pos2);
}

/** \brief Intercept output stream redirection.
 ** \param ostr the destination output stream
 ** \param pos a reference to the position to redirect
 */
template <typename YYChar>
std::basic_ostream<YYChar>& operator<<(std::basic_ostream<YYChar>& ostr,
                                       const Position& pos) {
  if (!pos.filename.empty()) ostr << pos.filename << ':';
  return ostr << pos.line << '.' << pos.column;
}

/// Two points in a source file.
class Location {
 public:
  /// Construct a location from \a b to \a e.
  Location(const Position& b, const Position& e) : begin(b), end(e) {}

  /// Construct a 0-width location in \a p.
  explicit Location(const Position& p = Position()) : begin(p), end(p) {}

  /// Construct a 0-width location in \a f, \a l, \a c.
  explicit Location(std::string_view f, unsigned l = 1u, unsigned c = 1u)
      : begin(f, l, c), end(f, l, c) {}

  /// Initialization.
  void initialize(std::string_view f, unsigned l = 1u, unsigned c = 1u) {
    begin.initialize(f, l, c);
    end = begin;
  }

  /** \name Line and Column related manipulators
   ** \{ */
 public:
  /// Reset initial location to final location.
  void step() { begin = end; }

  /// Extend the current location to the COUNT next columns.
  void columns(int count = 1) { end += count; }

  /// Extend the current location to the COUNT next lines.
  void lines(int count = 1) { end.lines(count); }
  /** \} */

 public:
  Position begin;
  Position end;
};

/// Join two locations, in place.
inline Location& operator+=(Location& res, const Location& end) {
  res.end = end.end;
  return res;
}

/// Join two locations.
inline Location operator+(Location res, const Location& end) {
  return res += end;
}

/// Add \a width columns to the end position, in place.
inline Location& operator+=(Location& res, int width) {
  res.columns(width);
  return res;
}

/// Add \a width columns to the end position.
inline Location operator+(Location res, int width) { return res += width; }

/// Subtract \a width columns to the end position, in place.
inline Location& operator-=(Location& res, int width) { return res += -width; }

/// Subtract \a width columns to the end position.
inline Location operator-(Location res, int width) { return res -= width; }

/// Compare two location objects.
inline bool operator==(const Location& loc1, const Location& loc2) {
  return loc1.begin == loc2.begin && loc1.end == loc2.end;
}

/// Compare two location objects.
inline bool operator!=(const Location& loc1, const Location& loc2) {
  return !(loc1 == loc2);
}

/** \brief Intercept output stream redirection.
 ** \param ostr the destination output stream
 ** \param loc a reference to the location to redirect
 **
 ** Avoid duplicate information.
 */
template <typename YYChar>
std::basic_ostream<YYChar>& operator<<(std::basic_ostream<YYChar>& ostr,
                                       const Location& loc) {
  unsigned end_col = 0 < loc.end.column ? loc.end.column - 1 : 0;
  ostr << loc.begin;
  if (loc.begin.filename != loc.end.filename)
    ostr << '-' << loc.end.filename << ':' << loc.end.line << '.' << end_col;
  else if (loc.begin.line < loc.end.line)
    ostr << '-' << loc.end.line << '.' << end_col;
  else if (loc.begin.column < end_col)
    ostr << '-' << end_col;
  return ostr;
}

}  // namespace internal
}  // namespace kipper