#include <vector>
#include <filesystem>
#include <iostream>

// Global constexpr arrays
constexpr std::array<int, 9> segments = {1, 2, 3, 4, 5, 12, 7, 8, 9};
constexpr std::array<int, 6> hashes = {0, 1,2,3,4,5};  // Reduced to 2 for the example

// Helper to generate arrays with sizes based on segments, and initialize first element with hash
template<size_t HashIdx, size_t... Indices>
constexpr auto createLayerHelper(std::index_sequence<Indices...>) {
    return std::make_tuple(
            std::array<int, segments[Indices]>{hashes[HashIdx]}...  // Create arrays with sizes from 'segments'
    );
}

// Function to create a layer of arrays at compile time, initializing with a specific hash
template<size_t HashIdx>
constexpr auto createLayer() {
    return createLayerHelper<HashIdx>(std::make_index_sequence<segments.size()>{});
}

// Function to create multiple layers (one for each hash)
template<size_t... HashIndices>
constexpr auto createMatrix(std::index_sequence<HashIndices...>) {
    return std::make_tuple(createLayer<HashIndices>()...);
}

// Helper function to print arrays
template<std::size_t N>
void printArray(const std::array<int, N>& arr) {
    for (const auto& elem : arr) {
        std::cout << elem << " ";
    }
    std::cout << "\n";
}

// Recursive function to print tuple elements (matrix rows)
template<std::size_t I = 0, typename... Ts>
typename std::enable_if<I == sizeof...(Ts)>::type
printTuple(const std::tuple<Ts...>&) {}

template<std::size_t I = 0, typename... Ts>
typename std::enable_if<I < sizeof...(Ts)>::type
printTuple(const std::tuple<Ts...>& t) {
    printArray(std::get<I>(t));
    printTuple<I + 1, Ts...>(t);
}

// Recursive function to print layers of arrays
template<std::size_t I = 0, typename... Layers>
typename std::enable_if<I == sizeof...(Layers)>::type
printMatrix(const std::tuple<Layers...>&) {}

template<std::size_t I = 0, typename... Layers>
typename std::enable_if<I < sizeof...(Layers)>::type
printMatrix(const std::tuple<Layers...>& matrix) {
    std::cout << "Layer " << I << ":\n";
    printTuple(std::get<I>(matrix));
    printMatrix<I + 1, Layers...>(matrix);
}

int main() {
    // Create the matrix at compile-time, layers initialized with values from hashes
    constexpr auto matrix = createMatrix(std::make_index_sequence<hashes.size()>{});

    // Print the matrix at runtime (printing cannot be constexpr)
    printMatrix(matrix);

    return 0;
}