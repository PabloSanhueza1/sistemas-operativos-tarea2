#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <atomic>
#include <chrono>
#include <string>
#include <unistd.h>
#include <fstream>
#include <mutex>
#include <condition_variable>

// esto para registrar cambios en el tamaño de la cola
// std::ofstream logFile("log.txt");

class ColaCircular
{
private:
    std::vector<int> queue;
    int maxSize;
    int head, tail, count;

    std::ofstream logFile;

    std::mutex mtx;
    std::condition_variable condVar;

    bool stop;

public:
    ColaCircular(int size) : maxSize(size), head(0), tail(0), count(0), stop(false)
    {
        // cabeza y cola comienzan al principio de la cola (0). Así como count es 0, porque obviamente, no hay elementos en la cola.
        queue.resize(size);
        logFile.open("log.txt");
    }

    ~ColaCircular()
    {
        if (logFile.is_open())
        {
            logFile.close();
        }
    }

    void insertar(int item)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (count == maxSize)
        {
            resize();
        }

        queue[tail] = item;
        tail = (tail + 1) % maxSize;
        count++;
        condVar.notify_one();
    }

    // eliminar un elemento de la cola
    int extraer()
    {
        std::unique_lock<std::mutex> lock(mtx);
        condVar.wait(lock, [this]
                     { return count > 0 || stop; });
        // Si no se cumple condicion anterior, la hebra permanecera en espera

        if (stop && count == 0)
        {
            // Cuando esto se cumpla, se debe detener el consumidor (no quedan elementos y ya nadie produce)
            return -1;
        }

        int item = queue[head];
        head = (head + 1) % maxSize;
        count--;

        return item;
    }

    void resize()
    {
        /*
            resize siempre se llama desde la sección crítica que ya está protegida desde insertar(),
            por lo que no es necesario un mutex adicional.
        */

        logChange("Duplicado");
        int newSize = maxSize * 2;
        std::vector<int> newQueue(newSize);
        for (int i = 0; i < count; ++i)
        {
            newQueue[i] = queue[head];
            head = (head + 1) % maxSize;
        }
        queue = std::move(newQueue);
        maxSize = newSize;
        head = 0;
        tail = count;
    }

    void logChange(const std::string &action)
    {
        // action puede ser 'Duplicado' o 'Reducido'
        logFile << "Cambio de tamaño de la cola: " << action << " a " << maxSize << " elementos." << std::endl;
    }

    void detener()
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
        condVar.notify_all(); // Todos los consumidores en espera deben ser despertados
    }
};

void productor(ColaCircular &cola, int id, int num_items)
{
    for (int i = 0; i < num_items; ++i)
    {
        int item = id * 100 + i;
        cola.insertar(item);
        // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // tiempo de producción
    }
}

void consumidor(ColaCircular &cola)
{
    while (true)
    {
        int item = cola.extraer();
        if (item == -1)
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{

    int cantProductores = 0;
    int cantConsumidores = 0;
    int initialSize = 0;
    int tiempoEspera = 0;

    int opt;
    while ((opt = getopt(argc, argv, "p:c:s:t:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            cantProductores = std::stoi(optarg);
            break;
        case 'c':
            cantConsumidores = std::stoi(optarg);
            break;
        case 's':
            initialSize = std::stoi(optarg);
            break;
        case 't':
            tiempoEspera = std::stoi(optarg);
            break;
        default:
            std::cerr << argv[0] << " -p <cantProductores> -c <cantConsumidores> -s <initialSize> -t <tiempoEspera>" << std::endl;
            return 1;
        }
    }
    
    ColaCircular cola(initialSize);

    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    int cantidadElementos = 100;

    // Crear productores y consumidores
    for (int i = 0; i < cantProductores; ++i)
    {
        productores.emplace_back(productor, std::ref(cola), i, cantidadElementos);
    }

    for (int i = 0; i < cantConsumidores; ++i)
    {
        consumidores.emplace_back(consumidor, std::ref(cola));
    }

    // Esperar a que los productores y consumidores terminen
    for (auto &productor : productores)
    {
        productor.join();
    }

    cola.detener();

    for (auto &consumidor : consumidores)
    {
        consumidor.join();
    }

    return 0;
}