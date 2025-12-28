#include <iomanip>
#include <iostream>
#include <nlnx/node.hpp>
#include <nlnx/nx.hpp>
#include <string>
#include <vector>

// Helper to split string by delimiter
std::vector<std::string> split(const std::string &str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  size_t start = 0, end = 0;
  while ((end = str.find(delimiter, start)) != std::string::npos) {
    token = str.substr(start, end - start);
    if (!token.empty())
      tokens.push_back(token);
    start = end + 1;
  }
  if (start < str.size()) {
    tokens.push_back(str.substr(start));
  }
  return tokens;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <file.nx> [path/to/node]"
              << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  std::string path = (argc > 2) ? argv[2] : "";

  try {
    nl::node file = nl::nx::add_file(filename);
    if (!file.valid()) {
      std::cerr << "Error: Could not load file " << filename << std::endl;
      return 1;
    }

    nl::node current = file;

    if (!path.empty()) {
      // Simple path traversal
      // Note: NoLifeNx resolve might handle this, but let's be explicit or use
      // resolve if available Looking at header, node::resolve(string) exists.
      current = current.resolve(path);
    }

    if (!current.valid()) {
      std::cerr << "Error: Path not found or invalid node." << std::endl;
      // If resolve fails it might return invalid node, checking valid() is
      // good. But let's try manual traversal if resolve is tricky or not doing
      // deep search Actually resolve logic: "Takes a '/' separated string, and
      // resolves the given path" So it should work.
      return 1;
    }

    std::cout << "Node: " << (path.empty() ? "Root" : path) << std::endl;
    std::cout
        << "Type: " << (int)current.data_type()
        << " (0=None, 1=Int, 2=Real, 3=String, 4=Vector, 5=Bitmap, 6=Audio)"
        << std::endl;
    std::cout << "Children: " << current.size() << std::endl;
    std::cout << "------------------------------------------------"
              << std::endl;

    // Print value if it's a value type
    switch (current.data_type()) {
    case nl::node::type::integer:
      std::cout << "Value (Int): " << (long long)current << std::endl;
      break;
    case nl::node::type::real:
      std::cout << "Value (Real): " << (double)current << std::endl;
      break;
    case nl::node::type::string:
      std::cout << "Value (String): " << (std::string)current << std::endl;
      break;
    case nl::node::type::vector:
      std::cout << "Value (Vector): " << current.x() << ", " << current.y()
                << std::endl;
      break;
    default:
      break;
    }

    std::cout << "------------------------------------------------"
              << std::endl;
    // List children
    for (auto child : current) {
      std::cout << std::left << std::setw(30) << child.name();
      std::cout << " [Type: " << (int)child.data_type() << "]";

      // Preview simple values
      switch (child.data_type()) {
      case nl::node::type::integer:
        std::cout << " = " << (long long)child;
        break;
      case nl::node::type::string:
        std::cout << " = " << (std::string)child;
        break;
      case nl::node::type::real:
        std::cout << " = " << (double)child;
        break;
      default:
        break;
      }
      std::cout << std::endl;
    }

  } catch (const std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
