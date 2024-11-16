#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unistd.h>
#include <string>
#include <algorithm>

void loadReferences(const std::string &filename, std::vector<int> &pageRefs)
{
    std::ifstream file(filename); // Abre el archivo en modo de lectura
    if (!file.is_open())
    {                                                                          // Verifica si el archivo no se abrió correctamente
        std::cerr << "Error al abrir el archivo de referencias." << std::endl; // Muestra un mensaje de error
        exit(1);                                                               // Termina el programa con un código de error
    }
    int page;
    while (file >> page)
    {                             // Lee cada número de página del archivo
        pageRefs.push_back(page); // Lo agrega al vector de referencias
    }
    file.close(); // Cierra el archivo después de leerlo
}

// Algoritmo FIFO
int simulateFIFO(const std::vector<int> &pageRefs, int numFrames)
{
    std::queue<int> frameQueue; // Cola para mantener el orden de llegada de páginas
    std::vector<int> frames;    // Vector para verificar las páginas en memoria
    int pageFaults = 0;         // Contador de fallos de página

    for (int page : pageRefs)
    { // Recorre cada referencia de página
        if (find(frames.begin(), frames.end(), page) == frames.end())
        {                 // Si la página no está en memoria
            pageFaults++; // Incrementa los fallos de página
            if (frameQueue.size() == numFrames)
            {                                                                   // Si la memoria está llena
                int pageToRemove = frameQueue.front();                          // Página más antigua
                frameQueue.pop();                                               // Elimina de la cola
                frames.erase(find(frames.begin(), frames.end(), pageToRemove)); // Elimina de memoria
            }
            frameQueue.push(page);  // Agrega la nueva página a la cola
            frames.push_back(page); // La registra en memoria
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo LRU
int simulateLRU(const std::vector<int> &pageRefs, int numFrames)
{
    std::vector<int> frames;
    int pageFaults = 0;

    for (int page : pageRefs)
    {                                                            // Recorre cada referencia de página
        auto it = std::find(frames.begin(), frames.end(), page); // Busca la página en memoria
        if (it == frames.end())
        {                 // Si la página no está en memoria
            pageFaults++; // Incrementa los fallos de página
            if (frames.size() == numFrames)
            {                                 // Si la memoria está llena
                frames.erase(frames.begin()); // Elimina la página menos usada (la primera)
            }
            frames.push_back(page); // Agrega la nueva página al final
        }
        else
        {
            frames.erase(it);       // Elimina la página existente
            frames.push_back(page); // La mueve al final (más recientemente usada)
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo Óptimo
int simulateOptimal(const std::vector<int> &pageRefs, int numFrames)
{
    std::vector<int> frames;
    int pageFaults = 0;

    for (int i = 0; i < pageRefs.size(); ++i)
    { // Recorre cada referencia de página
        int page = pageRefs[i];
        if (std::find(frames.begin(), frames.end(), page) == frames.end())
        {                 // Si no está en memoria
            pageFaults++; // Incrementa los fallos de página
            if (frames.size() == numFrames)
            { // Si la memoria está llena
                int farthest = -1, indexToReplace = -1;
                for (int j = 0; j < frames.size(); ++j)
                {
                    int nextUse = find(pageRefs.begin() + i + 1, pageRefs.end(), frames[j]) - pageRefs.begin();
                    if (nextUse > farthest)
                    { // Busca la página que se usará más tarde
                        farthest = nextUse;
                        indexToReplace = j;
                    }
                }
                frames.erase(frames.begin() + indexToReplace); // Reemplaza esa página
            }
            frames.push_back(page); // Agrega la nueva página
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo Clock
int simulateClock(const std::vector<int> &pageRefs, int numFrames)
{
    std::vector<int> frames(numFrames, -1);     // Vector inicializado con -1 para representar marcos vacíos
    std::vector<bool> useBit(numFrames, false); // Bits de uso para cada marco
    int pageFaults = 0, pointer = 0;            // Contador de fallos y puntero del reloj

    for (int page : pageRefs)
    { // Recorre cada referencia de página
        auto it = find(frames.begin(), frames.end(), page);
        if (it == frames.end())
        {                 // Si no está en memoria
            pageFaults++; // Incrementa los fallos de página
            while (useBit[pointer])
            { // Busca un marco con bit de uso 0
                useBit[pointer] = false;
                pointer = (pointer + 1) % numFrames; // Avanza el puntero
            }
            frames[pointer] = page;              // Reemplaza la página
            useBit[pointer] = true;              // Marca el bit de uso como 1
            pointer = (pointer + 1) % numFrames; // Avanza el puntero
        }
        else
        {                                       // Si está en memoria
            useBit[it - frames.begin()] = true; // Marca el bit de uso como 1
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

int main(int argc, char *argv[])
{
    int numFrames = 0;
    std::string algorithm;
    std::string filename;

    int opt;
    while ((opt = getopt(argc, argv, "m:a:f:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            numFrames = std::stoi(optarg);
            break;
        case 'a':
            algorithm = optarg;
            break;
        case 'f':
            filename = optarg;
            break;
        default:
            std::cerr << argv[0] << " -m <num_frames> -a <algoritmo> -f <archivo_referencias>" << std::endl;
            return 1;
        }
    }

    std::vector<int> pageRefs;
    loadReferences(filename, pageRefs); // Carga las referencias de página

    int pageFaults = 0;
    if (algorithm == "FIFO")
    {
        pageFaults = simulateFIFO(pageRefs, numFrames);
    }
    else if (algorithm == "LRU")
    {
        pageFaults = simulateLRU(pageRefs, numFrames);
    }
    else if (algorithm == "Optimal")
    {
        pageFaults = simulateOptimal(pageRefs, numFrames);
    }
    else if (algorithm == "Clock")
    {
        pageFaults = simulateClock(pageRefs, numFrames);
    }
    else
    {
        std::cout << "Algoritmo no reconocido. Use FIFO, LRU, Optimal o Clock." << std::endl;
        return 1;
    }

    std::cout << "Fallos de página: " << pageFaults << std::endl; // Imprime los fallos de página
    return 0;
}
