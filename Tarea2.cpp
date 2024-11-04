#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <atomic>

using namespace std;

class Monitor_circular
{
public:
    explicit Monitor_circular(int tope, ofstream &log_file) : tope(tope), log_file(log_file) {}
    void agregar(int x)
    {
        // Hacemos un lock y despues hacemos un wait en el caso de que se quiera agregar pero no hay espacio en la cola
        unique_lock<mutex> lock(mono);
        no_lleno.wait(lock, [this]
                      { return cola.size() < tam_max; });
        // Agregamos el item a la cola
        cola.push_back(x);
        cout << "Se produce el item: " << x << endl;
        ajuste();
        // En caso de haya una hebra consumidora esperando se notifica que ahora la cola no esta vacia para que consuma
        no_vacio.notify_one();
    }
    int sacar()
    {
        unique_lock<mutex> lock(mono);
        no_vacio.wait(lock, [this]
                      { return !cola.empty(); });
        // Tomamos el item que tenga al principio
        int item = cola.front();
        // Despues se borra y es consumido
        cola.erase(cola.begin());

        cout << "Se consume el item: " << item << endl;

        ajuste();
        no_lleno.nofity_one();
    }
    // El juste pedido cuando se llena por completo la cola o cuando esta al 25% llena la cola
    void ajuste()
    {
        if (cola.size() == tam_max)
        {
            tam_max *= 2;
            log_file << "Duplicado porque alcanzo tamanyo maximo" << tam_max << endl;
        }
        else if (cola.size() <= tam_max / 4)
        {
            tam_max /= 2;
            log_file << "El tamanyo de la cola esta al 25% " << tam_max << endl;
        }
    }
    // Es una forma para evitar que varios consumidores lleguen e intente consumir, haciendo que esperen (verificar elementos de la cola)
    bool vacio()
    {
        unique_lock<mutex> lock(mono);
        return cola.empty();
    }

private:
    vector<int> cola;
    int tam_max;
    mutex mono;
    condition_variable no_vacio;
    condition_variable no_lleno;
    ofstream &log_file;
};

void productor(&Monitor_circular monitor, int ident, atomic<bool> &termino)
{
}

int main()
{
    srand(static_cast<unsigned int>(time(NULL)));
}