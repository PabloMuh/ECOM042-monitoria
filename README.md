## Atividade 05 — LEDs independentes com um único timer

Construa um módulo de LED que faz **2 ou mais LEDs** piscarem em períodos diferentes e independentes usando **apenas 1 timer** do kernel, simulado no `native_sim` ou com placa real. Na placa real, mandar vídeo mostrando o funcionamento.

É o desenho de um driver de LED de produto: o resto do código pede *"""LED 1 piscando devagar"""* e não sabe nada de timer. Um único `k_timer` é multiplexado entre os LEDs.

**INTERFACE:**

Crie um módulo próprio de LED (`.h`/`.c`, nomes livres) que ofereça, no mínimo:

* **Inicialização**: verifica se os GPIOs estão prontos e configura todos os LEDs como saída, **desligados**. Retorna erro se algum controlador não estiver pronto. Deve poder ser chamada de novo sem corromper o timer.
* **Ligar**, **desligar** e **alternar** (toggle) um LED. Cada uma cancela o pisca em andamento daquele LED.
* **Piscar** um LED com um período escolhido entre pelo menos 3 padrões (ex.: lento, padrão e rápido).
* Uma forma de aplicar qualquer ação a todos os LEDs de uma vez.
* Argumento inválido (LED ou padrão fora do intervalo) retorna erro (`-EINVAL`) e registra com `LOG_ERR`.
* Períodos dos padrões configuráveis por **Kconfig**, sem número mágico no `.c`.

**REQUISITOS:**

1. Timer

* **Exatamente 1 `struct k_timer`** no projeto. Proibido um timer por LED, e proibido `k_sleep`/`k_msleep` para temporizar piscadas dentro do módulo.
* Timer **one-shot rearmado dinamicamente**.

2. Callback e LEDs

* O callback roda em contexto de ISR e deve ser **curto**: só GPIO, deadlines e rearme, **sem `printk`/log**.
* LEDs **independentes**: iniciar, parar ou mudar o pisca de um não altera a fase nem o período dos outros.
* Cada LED guarda o **seu próprio estado** (padrão atual, período, deadline, se está piscando) numa estrutura indexada pelo LED. LED novo não pode exigir função nova.

**ARQUITETURA**

* **Command Pattern / table-driven**:  Proibido `if/else` ou `switch` por estado.
* **"Um ou todos" em um lugar só**: trate "todos os LEDs" num helper, sem repetir o laço em cada função.
* LEDs obtidos pelo **devicetree** (aliases `led0`, `led1`, …), nunca pino ou controlador fixo no código.

### Testes unitários

10. **Obrigatório**: testes unitários com **ztest** em app de teste próprio (ex.: `tests/unit/` com `CMakeLists.txt`, `prj.conf`, `app.overlay`, `testcase.yaml` e `src/`), rodando no `native_sim` via `west twister`.
11. O overlay de teste declara um **`zephyr,gpio-emul`** com `gpio-leds` e os aliases dos LEDs, para observar o nível dos pinos com `gpio_emul_output_get`.
12. Cobertura mínima:
    * inicialização retorna sucesso e todos os LEDs começam desligados;
    * inicialização falha com o controlador não pronto (ex.: `zephyr,deferred-init`);
    * ligar, desligar e alternar mudam o nível do pino;
    * piscar faz o pino ser observado **ligado e desligado**;
    * dois LEDs com períodos diferentes: no mesmo intervalo, o mais rápido troca mais vezes que o mais lento;
    * ligar/desligar/alternar **cancelam** um pisca em andamento;
    * a ação em """todos os LEDs""" afeta todos;
    * LED e padrão inválidos retornam erro.
