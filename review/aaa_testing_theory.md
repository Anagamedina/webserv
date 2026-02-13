# El Patrón AAA (Arrange-Act-Assert) en Testing

Si eres nuevo en testing, **AAA** es el acrónimo más importante que debes aprender. Es una forma estandarizada de organizar el código de **cada uno de tus tests**.

Imagina un test como un experimento científico o una receta de cocina.

## Las 3 Fases

### 1. Arrange (Preparar)
**"Poner los ingredientes en la mesa"**
Aquí configuras todo lo necesario antes de ejecutar la acción.
*   Instancias los objetos.
*   Configuras variables.
*   Estableces el estado inicial.

### 2. Act (Actuar)
**"Cocinar / Ejecutar la acción"**
Aquí llamas a la función o método específico que quieres probar.
*   **Regla de oro:** Idealmente debería ser **una sola línea de código**. Si hay muchas líneas aquí, quizás tu test es demasiado complejo.

### 3. Assert (Verificar)
**"Probar la comida"**
Aquí compruebas si el resultado es exactamente el que esperabas.
*   Comparas el resultado obtenido (`actual`) con el esperado (`expected`).
*   Verificas si cambió el estado de un objeto.

---

## Analogía Clásica: El Cajero Automático (ATM)

Esta es una analogía muy común en la literatura de testing para explicar el cambio de estado.

1.  **Arrange (Preparar):**
    *   Llegas al cajero.
    *   **Estado inicial:** Tienes $500 en tu cuenta y la tarjeta insertada.
    
2.  **Act (Actuar):**
    *   Pulsas el botón de "Retirar $100".
    *   (Esta es la única acción que estamos probando).

3.  **Assert (Verificar):**
    *   **Verificación 1 (Salida):** El cajero te escupe $100.
    *   **Verificación 2 (Estado):** Tu saldo en pantalla ahora dice $400.
    *   **Verificación 3 (Efecto secundario):** Te devuelve la tarjeta.

---

## Ejemplo 1: Una cuenta bancaria (Muy simple)

Imagina una clase `CuentaBancaria`. Queremos probar el método `depositar()`.

```cpp
TEST_CASE("Depositar dinero aumenta el saldo", "[banco]") {
    
    // --- 1. ARRANGE (Preparar) ---
    // Creamos la cuenta y le damos un saldo inicial conocido
    CuentaBancaria miCuenta;
    miCuenta.setSaldo(100); 

    // --- 2. ACT (Actuar) ---
    // Hacemos SOLO la acción que queremos probar
    miCuenta.depositar(50);

    // --- 3. ASSERT (Verificar) ---
    // Comprobamos: ¿Tengo 150? (100 inicial + 50 deposito)
    REQUIRE(miCuenta.getSaldo() == 150);
}
```

---

## Ejemplo 2: Aplicado a tu Proyecto (ServerConfig)

Veamos cómo se aplica esto a tu clase [ServerConfig](file:///home/daruuu/CLionProjects/webserv-fork/src/config/ServerConfig.cpp#9-15) para probar si guarda bien el puerto.

```cpp
TEST_CASE("Configuración del puerto del servidor", "[server]") {
    
    // --- 1. ARRANGE ---
    // Creamos el objeto vacío
    ServerConfig servidor;
    int puertoDeseado = 8080;

    // --- 2. ACT ---
    // Ejecutamos el setter
    servidor.setPort(puertoDeseado);

    // --- 3. ASSERT ---
    // Verificamos que el getter nos devuelva lo mismo
    REQUIRE(servidor.getPort() == 8080);
}
```

## ¿Por qué es útil separarlo así?

1.  **Legibilidad:** Cualquiera que lea tu test sabe dónde mirar.
    *   ¿Cómo estaba el sistema? -> Mira *Arrange*.
    *   ¿Qué hizo? -> Mira *Act*.
    *   ¿Qué falló? -> Mira *Assert*.
2.  **Depuración:** Si el test falla en *Arrange*, sabes que tu configuración está mal. Si falla en *Act*, tu código explotó. Si falla en *Assert*, tu lógica da un resultado incorrecto.

## Errores Comunes de Principiante

❌ **Mezclar fases:**
```cpp
// MAL HECHO (Difícil de leer)
ServerConfig s;
s.setPort(80);
REQUIRE(s.getPort() == 80); // Assert mezclado
s.setPort(90);              // Otro Act mezclado
REQUIRE(s.getPort() == 90);
```

✅ **Haz tests separados:**
Si quieres probar otra cosa, **haz otro `SECTION` u otro `TEST_CASE`**. Mantén cada test limpio y enfocado en una sola cosa.
