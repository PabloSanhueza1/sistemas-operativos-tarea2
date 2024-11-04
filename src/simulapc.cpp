#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <atomic>
#include <chrono>
#include <string>

// esto para registrar cambios en el tamaño de la cola
std::ofstream logFile("log.txt");

class ColaCircular
{
private:
    std::vector<int> queue;
    int maxSize;
    int head, tail, count;

public:
    ColaCircular(int size) : maxSize(size), head(0), tail(0), count(0)
    {
        // cabeza y cola comienzan al principio de la cola (0). Así como count es 0, porque obviamente, no hay elementos en la cola.
        queue.resize(size);
    }

    void insertar(int item)
    {
    }

    // eliminar un elemento de la cola
    int extraer()
    {
    }

    void resize(int newSize)
    {
    }

    void logChange(const std::string &action)
    {
        // action puede ser 'Duplicado' o 'Reducido'
        logFile << "Cambio de tamaño de la cola: " << action << " a " << maxSize << " elementos." << std::endl;
    }
};

void productor(ColaCircular &cola)
{
}

void consumidor(ColaCircular &cola)
{
}

int main()
{
    int initialSize = 10;
    ColaCircular cola(initialSize);

    std::thread prod1(productor, std::ref(cola));
    std::thread cons1(consumidor, std::ref(cola));

    prod1.join();
    cons1.join();

    logFile.close();

    return 0;
}