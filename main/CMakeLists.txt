if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/secrets.h")
    message(FATAL_ERROR
        "\nFalta main/secrets.h\n"
        "Copia main/secrets.example.h como main/secrets.h y llena tus datos de WiFi y Adafruit IO.\n")
endif()

idf_component_register(
    SRCS "main.c" "wifi.c" "mqtt_aio.c" "dht22.c" "suelo.c" "rele.c" "riego_logic.c"
    INCLUDE_DIRS "."
)
