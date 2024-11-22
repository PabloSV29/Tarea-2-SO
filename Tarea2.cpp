#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <atomic>

using namespace std;

class Monitor_circular
{
public:
    // Para definir los tamanyos maximos y el archivo que se guardara
    explicit Monitor_circular(size_t max_size, ofstream &log_file) : max_size(max_size), log_file(log_file) {}
    void agregar(int x)
    {
        // Hacemos un lock y despues hacemos un wait en el caso de que se quiera agregar pero no hay espacio en la cola
        unique_lock<mutex> lock(mono);
        no_lleno.wait(lock, [this]
                      { return cola.size() < max_size; });
        // Agregamos el item a la cola
        cola.push_back(x);
        cout << "Se produce el item: " << x << endl;
        log_file << "Se produce el item: " << x << endl;
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
        log_file << "Se consume el item: " << item << endl;
        ajuste();
        no_lleno.notify_one();
        return item;
    }
    // El juste pedido cuando se llena por completo la cola o cuando esta al 25% llena la cola
    void ajuste()
    {
        if (cola.size() == max_size)
        {
            max_size *= 2;
            log_file << "Duplicado porque alcanzo tamanyo maximo " << max_size << endl;
        }
        else if (cola.size() <= max_size / 4 && max_size > 1)
        {
            max_size /= 2;
            log_file << "El tamanyo de la cola esta al 25% " << max_size << endl;
        }
    }
    // Es una forma para evitar que varios consumidores lleguen e intente consumir, haciendo que esperen (verificar elementos de la cola)
    bool vacio()
    {
        unique_lock<mutex> lock(mono);
        return cola.empty();
    }

private:
    size_t max_size; // Variable que me permite hacer los ajustes del tamanyo de la cola circular
    vector<int> cola;
    // int tam_max;
    mutex mono;
    condition_variable no_vacio; // Me permite saber cuando la cola circular hay elementos
    condition_variable no_lleno; // Me permite saber si en la cola no hay elementos
    ofstream &log_file;
};

void productor(Monitor_circular &monitor, int ident, atomic<bool> &termino)
{
    while (!termino)
    {
        int x = rand() % 100 + 1; // Asigna un valor para el item
        monitor.agregar(x);
      
    }
}
void consumidor(Monitor_circular &monitor, int ident, atomic<bool> &termino, int tiempo_limite)
{
    auto t_inicio = chrono::steady_clock::now(); // Iniciamos el tiempo para saber cuando llevan los consumidores

    while (!termino)
    {
        // printf("Sigo aqui");
        if (!monitor.vacio()) // Si en la cola hay elementos, sacamos el elemento y le damos un tiempo de espera para el consumidor en milisegundos
        {
            int x = monitor.sacar();
            // No se si es necesario esta parte
            int espera_consumidor = rand() % 901 + 100;
            this_thread::sleep_for(chrono::milliseconds(espera_consumidor));
        }
        // Calculamos el tiempo que ha transcurrido y seguimos hasta que lleguemos al tiempo limite que se asigna por la terminal
        auto t_actual = chrono::steady_clock::now();
        auto t_pasado = chrono::duration_cast<chrono::seconds>(t_actual - t_inicio);
        if (t_pasado.count() >= tiempo_limite)
        {
            termino = true;
        }
    }
}

int main(int argc, char *argv[])
{
    srand(static_cast<unsigned int>(time(NULL)));
    // Comprobamos que se escriba correctamente en el formato pedido
    if (argc < 9)
    {
        cerr << "Uso: " << argv[0] << " -p (numero_productores) -c (numero_consumidores) -s (tamanyo_inicial) -t (tiempo_limite)" << endl;
        return 1;
    }
    // Creamos las variables, en el que su asignacion sera por medio de parametros escritos por la terminal
    int n_productores = atoi(argv[2]);
    int n_consumidores = atoi(argv[4]);
    size_t tamanyo_inicial = atoi(argv[6]);
    int tiempo_limite = atoi(argv[8]);

    // Creamos el archivo "log.txt" que nos guardara el progreso cuando se ejecute el programa
    ofstream log_file("log.txt");

    // Abrimos el archivo
    if (!log_file.is_open())
    {
        cerr << "Error al abrir el archivo de log." << endl;
        return 1;
    }
    // Creaoms las variables relevantes que son el monitor basado en una cola circular y una variable atomica que me permite alinear los consumidores con los productores
    Monitor_circular mc(tamanyo_inicial, log_file);
    atomic<bool> termino(false);
    // Creamos un vector tanto de productores como de consumidores
    vector<thread> productores;
    vector<thread> consumidores;

    /*Para ambos for se crean las hebras respectivas para el productor como su consumidor, con los parametros requeridos.*/
    for (int i = 0; i < n_productores; i++)
    {
        productores.push_back(thread(productor, ref(mc), i, ref(termino)));
    }
    for (int i = 0; i < n_consumidores; i++)
    {
        consumidores.push_back(thread(consumidor, ref(mc), i, ref(termino), tiempo_limite));
    }
    // Realizamos 2 for mas en el que vamos a hacer de que se esperen entre todas las hebras para que se sincronicen y siga cuando hayan todos terminado.
    for (auto &productor : productores)
    {
        productor.join();
    }

    for (auto &consumidor : consumidores)
    {
        consumidor.join();
    }
    // Se cierra el archivo
    log_file.close();
    return 0;
}