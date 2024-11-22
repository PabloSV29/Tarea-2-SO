#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <algorithm> 
class PageTableEntry {
public:
    int virtualPage;
    int physicalFrame; // -1 indica que no está en memoria
    bool valid;
    int loadedTime; //  Tiempo en que la página fue cargada(para FIFO)
    int lastUsedTime;   // Tiempo del último uso (para LRU)
    bool referenceBit; // Bit de referencia  LRU

    PageTableEntry(int vp) : virtualPage(vp), physicalFrame(-1), valid(false), loadedTime(0), lastUsedTime(0), referenceBit(false) {}

    // Sobrecarga del operador de inserción para imprimir la entrada
    friend std::ostream& operator<<(std::ostream& os, const PageTableEntry& entry) {
        os << "Page " << entry.virtualPage << ": Frame ";
        if (entry.physicalFrame == -1) {
            os << "Not Loaded";
        } else {
            os << entry.physicalFrame;
        }
        os << ", Valid: " << (entry.valid ? "true" : "false")
           << ", Loaded: " << entry.loadedTime
           << ", Last Used: " << entry.lastUsedTime
           << ", Ref Bit: " << (entry.referenceBit ? "true" : "false");
        return os;
    }
};

class MemorySimulator {
private:
    int numFrames;
    //Tabla hash con chaining
    std::vector<std::list<PageTableEntry> > pageTable; 
    std::vector<int> physicalMemory; // Vector memoria física
    int pageFaults;
    int currentTime; // Reloj para simular el tiempo
    std::string algorithm; // Algoritm o de reemplazo

    // Función hash simple
    int hashFunction(int virtualPage) {
        return virtualPage % pageTable.size();
    }

    // Métodos para los algoritmos de reemplazo
    int fifoReplace();
    int lruReplace();
    int relojsimpleReplace();

public:
    MemorySimulator(int frames, std::string algo) : numFrames(frames), pageTable(frames * 2), physicalMemory(frames, -1), pageFaults(0), currentTime(0), algorithm(algo) {}

    void run(const std::string& filename);

    int getPageFaults() const {
        return pageFaults;
    }
};

void MemorySimulator::run(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Leer la unica linea con las referencias
    std::stringstream ss(line);
    int virtualPage;

    while (ss >> virtualPage) {
        currentTime++;

        int index = hashFunction(virtualPage);
        auto& chain = pageTable[index];

        auto it = chain.begin();    // buscar la pagina
        for (; it != chain.end(); ++it) {
            if (it->virtualPage == virtualPage) {
            break; 
        }
    }

        if (it != chain.end() && it->valid) {
            // Página encontrada y válida 
            it->lastUsedTime = currentTime;
            it->referenceBit = true;
        } else {
            pageFaults++;

            int freeFrame = -1;
            for (int i = 0; i < numFrames; ++i) {
                if (physicalMemory[i] == -1) {
                    freeFrame = i;
                    break;
                }
            }

            if (freeFrame != -1) {
                // Hay un marco libre
                physicalMemory[freeFrame] = virtualPage;

                if (it == chain.end()) {
                    // La página no estaba, la agregamos
                    chain.emplace_back(virtualPage); // Usamos emplace_back para eficiencia
                    it = std::prev(chain.end());    // ctualiza para que apunte a la nueva entrada
                }
                
                it->physicalFrame = freeFrame;
                it->valid = true;
                it->loadedTime = currentTime;
                it->lastUsedTime = currentTime;
                it->referenceBit = true;

            } else {
                int replacedFrame = -1;
                if (algorithm == "FIFO") {
                    replacedFrame = fifoReplace();
                } else if (algorithm == "LRU") {
                    replacedFrame = lruReplace();
                } else if (algorithm == "LRU Reloj simple") {
                    replacedFrame = relojsimpleReplace();
                }


                 for (auto& list : pageTable) {
                    for (auto& entry : list) {
                        if (entry.physicalFrame == replacedFrame) {
                            entry.valid = false;
                            entry.physicalFrame = -1;
                            break; 
                        }
                    }
                }

                // Cargar la nueva página
                physicalMemory[replacedFrame] = virtualPage;
                if (it == chain.end()) {
                   chain.emplace_back(virtualPage);
                   it = std::prev(chain.end());
                }
                it->physicalFrame = replacedFrame;
                it->valid = true;
                it->loadedTime = currentTime;
                it->lastUsedTime = currentTime;
                it->referenceBit = true;
            }
        }
    }
}

// Implementaciones de los algoritmos de reemplazo

int MemorySimulator::fifoReplace() {
    // Encuentra la página cargada hace más tiempo y retorna su marco
    int oldestFrame = 0;
    int oldestTime = pageTable[hashFunction(physicalMemory[0])].front().loadedTime;
    for (int i = 1; i < numFrames; ++i) {
        int vp = physicalMemory[i];
        int index = hashFunction(vp);
        for (const auto& entry : pageTable[index]) { // Buscar la entrada correspondiente
            if (entry.physicalFrame == i) {
                if (entry.loadedTime < oldestTime) {
                    oldestTime = entry.loadedTime;
                    oldestFrame = i;
                }
                break; 
            }
        }
    }
    return oldestFrame;
}

int MemorySimulator::lruReplace() {

    int leastRecentlyUsedFrame = 0;
    int leastRecentlyUsedTime = pageTable[hashFunction(physicalMemory[0])].front().lastUsedTime;
    for (int i = 1; i < numFrames; ++i) {
        int vp = physicalMemory[i];
        int index = hashFunction(vp);
        for (const auto& entry : pageTable[index]) { 
            if (entry.physicalFrame == i) {
                if (entry.lastUsedTime < leastRecentlyUsedTime) {
                    leastRecentlyUsedTime = entry.lastUsedTime;
                    leastRecentlyUsedFrame = i;
                }
                break; 
            }
        }
    }
    return leastRecentlyUsedFrame;
}

int MemorySimulator::relojsimpleReplace() {
    static int clockHand = 0; // Mantener el puntero del reloj entre llamadas

    while (true) {
        int vp = physicalMemory[clockHand];
        int index = hashFunction(vp);
        for (auto& entry : pageTable[index]) {
            if (entry.physicalFrame == clockHand) {
                if (!entry.referenceBit) {
                    clockHand = (clockHand + 1) % numFrames; // Mover el puntero para la próxima vez
                    return entry.physicalFrame; 
                } else {
                    entry.referenceBit = false;
                }
                break; 
            }
        }
        clockHand = (clockHand + 1) % numFrames; // Avanzar si la página tiene el bit de referencia
    }
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] << " -m <num_frames> -a <algoritmo> -f <archivo_referencias>" << std::endl;
        return 1;
    }

    int numFrames = 0;
    std::string algorithm;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-m") {
            if (i + 1 < argc) {
                numFrames = std::stoi(argv[++i]);
            }
        } else if (arg == "-a") {
            if (i + 1 < argc) {
                algorithm = argv[++i];
            }
        } else if (arg == "-f") {
            if (i + 1 < argc) {
                filename = argv[++i];
            }
        }
    }

    if (numFrames <= 0 || algorithm.empty() || filename.empty()) {
        std::cerr << "Argumentos invalidos." << std::endl;
        return 1;
    }

    MemorySimulator simulator(numFrames, algorithm);
    simulator.run(filename);

    std::cout << "Numero total de fallos de pagina: " << simulator.getPageFaults() << std::endl;

    return 0;
}
