# Tipos de stringstream en C++98

En C++98, la librería `<sstream>` proporciona clases para tratar las cadenas de texto (`std::string`) como si fueran flujos de datos (similar a `cin` y `cout`). Aquí tienes una breve explicación de cada uno:

## 1. `std::istringstream` (Input String Stream)
*   **Para qué sirve:** Solo para **LEER** datos de un string.
*   **Cómo funciona:** Toma un string y permite extraer información de él (números, palabras) usando el operador `>>`.
*   **Caso de uso típico:** Parsear una línea de configuración, dividir una frase en palabras o convertir un texto ("123") a un número entero (`int`).

## 2. `std::ostringstream` (Output String Stream)
*   **Para qué sirve:** Solo para **ESCRIBIR** datos en un string.
*   **Cómo funciona:** Permite "imprimir" diferentes tipos de datos (números, texto, booleanos) dentro de un objeto usando el operador `<<`. Luego obtienes el resultado final con el método `.str()`.
*   **Caso de uso típico:** Formatear mensajes de log, construir strings complejos o convertir un número (`int`, `float`) a texto.

## 3. `std::stringstream` (Input/Output String Stream)
*   **Para qué sirve:** Para **LEER Y ESCRIBIR** al mismo tiempo.
*   **Cómo funciona:** Combina las capacidades de los dos anteriores. Puedes meter datos con `<<` y sacarlos con `>>`.
*   **Caso de uso típico:** Transformaciones complejas o cuando necesitas un buffer intermedio de uso general.

---

> **Resumen rápido:**
> *   ¿Quieres **sacar** información de un texto? Usa `istringstream`.
> *   ¿Quieres **armar** un texto con variables? Usa `ostringstream`.
> *   ¿Necesitas **ambas** cosas? Usa `stringstream`.
