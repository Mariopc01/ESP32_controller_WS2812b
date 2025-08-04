# ESP32_controller_WS2812b
Microcontrolador ESP32 controlando uma fita de Led endereçável WS2812b.

Funcionalidades: Projeto destina-se a criar uma iluminação diária alternando as cores conforme os dias da semana:
Blue - Domingo (0)
Yellow - Segunda-feira (1)
DeepPink - Terça-feira (2) - 'Rosa' mais vibrante
White - Quarta-feira (3)
Green - Quinta-feira (4)
Rubi - Sexta-feira (5) - 'Rubi' (usando valor RGB direto para um tom de vermelho profundo)
Violet - Sábado (6) - 'Violeta

A data e hora são atualizados pela internet através do Wi-fi. Na programação foi adicionado um modo no qual todo o dia 1 de janeiro durante o dia inteiro, a cada 1 hora seja feita uma animação pelo período de 1 minuto e nos outros dias do ano a cada 1 hora pisque suavemente por 7 vezes como se fosse um relógio.
Foram adicionados 3 botões push, um deles é o Modo, que que, quando pressionado uma vez ele interrompa a programação e inicia uma apresentação fazendo com que a fita pisque de 7 formas diferentes sendo que cada forma durará 30s e para interromper e voltar ao estado anterior basta pressionar o botão mais uma vez. 
Os outros 2 botões são para controlar o brilho da fita de Led WS2812b. 
A fita WS2812b possui 60 leds e cada led consome no máximo 60 miliamperes (o consumo total da fita no brilho máximo é de 3,6 Amperes), sendo assim, tive que limitar o brilho do led a no máximo 50%, ja que a fonte de alimentação é um carregador de celular que fornece 5 Volts e 2 Ampreres, com essa limitação a fita irá consumir no máximo 1,8 Amperes do carregador evitando assim que o queime além de aumentar a vida útil da fita.


Hardware Utilizado: 
- ESP32 Dev Kit
- Fita de LED WS2812B
- Botões Push
- Módulo RTC para salvar a data e hora
- Fonte de 5V 2A (carregador de celular).

Instruções de Montagem no ESP32: 
A fita WS2812b deve ser conectada ao GPIO 2

Botão de Modo: Um terminal no GPIO 23 e o outro no GND.

Botão de Aumentar Brilho: Um terminal no GPIO 22 e o outro no GND.

Botão de Diminuir Brilho: Um terminal no GPIO 21 e o outro no GND.

Módulo RTC: O SDA do módulo RTC deve ser conectado ao GPIO 16 do ESP32 e o pino SCL deve estar conectado ao GPIO 17 do ESP32.

A alimentação do carregador deverá ser conectada ao +5VCC e GND.


Instruções de Uso: 
Botão Modo: Serve para intercalar entre a programação normal e o modo de apresentação.
Botões + e - : Servem para aumentar ou diminuir o brilho da fita WS2812b
