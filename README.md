# Global Solution – Integração IoT com ESP32, Node-RED e Oracle

Projeto da Global Solution (2TDS) que integra um dispositivo **ESP32** com um backend em **Node-RED** e um banco de dados **Oracle XE**.  
O objetivo é capturar leituras de um “sensor” (simulado), enviá-las via HTTP para o Node-RED e gravar os dados na tabela `LEITURA_IOT` no banco.

---

## Descrição do Projeto – Sistema Inteligente de Inventário e Rastreamento IoT de Ativos de TI

O projeto propõe o desenvolvimento de um Sistema Inteligente de Inventário, Empréstimos e Rastreamento IoT de Ativos de TI, com automação baseada em dispositivos ESP32, integração via MQTT e um banco de dados Oracle completo para controle operacional e histórico.
O objetivo é modernizar a gestão de ativos corporativos — como notebooks, headsets, tablets e dispositivos móveis — oferecendo rastreabilidade em tempo real, automação de processos, segurança, redução de perdas e melhoria direta na experiência dos colaboradores.

---

## 1. Visão geral da arquitetura

1. **ESP32**
   - Conectado à mesma rede Wi‑Fi do notebook/PC.
   - Ao pressionar um botão físico, gera leituras **fake** de acelerômetro e giroscópio.
   - Envia um **HTTP POST** em JSON para o Node-RED.

2. **Node-RED**
   - Expõe um endpoint REST: **`POST /api/leituras-iot`**.
   - Converte o JSON recebido em objeto.
   - Normaliza os campos (tipos numéricos, strings, valores padrão).
   - Executa um `INSERT` na tabela `LEITURA_IOT`, vinculando a leitura a um dispositivo da tabela `DISPOSITIVO_IOT` usando o campo `IDENTIFICADOR_HW`.

3. **Banco Oracle XE (GSDB)**
   - Roda em container (Docker) com Service Name **GSDB**.
   - Usuário de aplicação: `GSUSER` / senha: `gspassword` (ou conforme configurado).
   - Contém as tabelas da solução (vendas, contábil, IoT etc.), criadas pelo script SQL da GS.

---

## 2. Requisitos

- **Hardware**
  - ESP32 DevKit (ou equivalente) com um botão físico disponível.

- **Software – Backend**
  - Node.js + Node-RED instalados.
  - Pacote Node-RED para Oracle:
    ```bash
    npm install node-red-contrib-oracledb-mod
    ```
  - **Oracle Instant Client 64 bits** instalado e configurado no PATH do Windows.
  - Docker rodando o container Oracle XE descrito na GS (service `GSDB`).

- **Software – Firmware**
  - Arduino IDE ou PlatformIO para compilar e gravar o `.ino` no ESP32.

---

## 3. Estrutura do projeto

Sugestão de organização de pastas do repositório:

```text
/esp32
  esp32_gs2_iot.ino          # Firmware do ESP32 (código-fonte)
/node-red
  flows_gs2.json             # Flow original da GS
  flows_gs3.json             # Flow ajustado para integração IoT
/database
  gs_schema.sql              # Script de criação das tabelas/objetos Oracle
README.md                    # Este arquivo
```

---

## 4. Banco de dados Oracle (GSDB)

### 4.1. Parâmetros de conexão

- **Host**: IP da máquina/VM onde o container Oracle está rodando (ex.: `9.169.156.28`)
- **Porta**: `1521`
- **Service Name (Nome do Serviço)**: `GSDB`
- **Usuário**: `GSUSER`
- **Senha**: `gspassword`

Esses dados são os mesmos usados no SQL Developer.

### 4.2. Criação de objetos

O script `gs_schema.sql` contém toda a modelagem necessária, incluindo:

- Tabelas de negócio (clientes, contas, lançamentos, etc.).
- Tabelas de IoT:
  - `DISPOSITIVO_IOT`
  - `LEITURA_IOT`
- Sequences e triggers (por exemplo, `SEQ_LEITURA_IOT` e trigger de auto‑incremento).

Execute o script com o usuário `GSUSER` após subir o container Oracle.

---

## 5. Flow Node-RED

### 5.1. Importando o flow

1. Inicie o Node-RED:
   ```bash
   node-red
   ```
2. Acesse o editor em `http://localhost:1880` (ou `http://SEU_IP:1880`).
3. Menu ☰ → **Import** → cole o conteúdo do arquivo `flows_gs3.json` → **Import** → **Deploy**.

### 5.2. Principais nós do fluxo **GS2 IoT Oracle**

- **HTTP In** – `POST /api/leituras-iot`  
  Recebe o JSON enviado pelo ESP32 (ou pelo Postman).

- **JSON → Objeto**  
  Converte o corpo da requisição para `msg.payload` como objeto JavaScript.

- **Function – “Normaliza payload”**  
  - Garante que todos os campos esperados existam:
    - `identificador_hw`
    - `acc_x`, `acc_y`, `acc_z`
    - `gyro_x`, `gyro_y`, `gyro_z`
    - `movimentado`, `choque`
    - `estado_ativo`, `observacao`
  - Converte textos para `Number` onde necessário.
  - Armazena o payload original em `msg._originalPayload` para devolver na resposta.

- **Oracle Node – “INSERT leitura_iot”**  
  Executa o comando SQL (exemplo simplificado):
  ```sql
  INSERT INTO leitura_iot (
    id_leitura,
    dispositivo_id,
    data_leitura,
    acc_x, acc_y, acc_z,
    gyro_x, gyro_y, gyro_z,
    movimentado,
    choque,
    estado_ativo,
    observacao
  )
  VALUES (
    seq_leitura_iot.NEXTVAL,
    (SELECT id_disp FROM dispositivo_iot WHERE identificador_hw = :identificador_hw),
    SYSDATE,
    :acc_x, :acc_y, :acc_z,
    :gyro_x, :gyro_y, :gyro_z,
    :movimentado, :choque,
    :estado_ativo, :observacao
  );
  ```

- **Function – “Monta resposta HTTP”**  
  Monta um JSON de retorno com:
  ```json
  {
    "status": "ok",
    "mensagem": "Leitura IoT gravada no Oracle com sucesso",
    "dadosRecebidos": { ... }
  }
  ```

- **HTTP Response**  
  Envia a resposta para quem chamou o endpoint (ESP32 ou Postman).

- **Debug Nodes**  
  Mostram o payload normalizado e o retorno do Oracle na aba **Depurar** do Node-RED.

### 5.3. Configuração do servidor Oracle no Node-RED

No nó de configuração `oracle-server`:

- **Host**: IP do container Oracle (ex.: `9.169.156.28`)
- **Port**: `1521`
- **DB/Service**: `GSDB`
- **Usuário/Senha**: `GSUSER` / `gspassword`
- **Instant Client Path**: caminho da pasta do Oracle Instant Client, por exemplo:
  ```text
  C:\oracle\instantclient_23_0
  ```

Após salvar a configuração, clique em **Deploy** novamente.

---

## 6. Firmware do ESP32 (`esp32_gs2_iot.ino`)

### 6.1. Parâmetros principais

No código `.ino` existem algumas constantes que devem ser ajustadas:

```cpp
const char* WIFI_SSID     = "NOME_DA_REDE";
const char* WIFI_PASSWORD = "SENHA_DA_REDE";

// IP da máquina onde o Node-RED está rodando
const char* NODE_RED_URL  = "http://192.168.X.X:1880/api/leituras-iot";

// Identificador de hardware cadastrado em DISPOSITIVO_IOT.IDENTIFICADOR_HW
const char* DEVICE_ID     = "ESP32-ABC-001";

// Pino do botão utilizado para disparar a leitura
const int BUTTON_PIN      = 0; // ajustar de acordo com a placa
```

### 6.2. Comportamento

1. O ESP32 conecta ao Wi‑Fi e mostra o IP no Serial Monitor.
2. Em `loop()`, ele monitora o botão.
3. Quando o botão é pressionado, chama `enviaLeituraFake()`:
   - Gera valores aleatórios coerentes para `acc_x`, `acc_y`, `acc_z` e giroscópio.
   - Define `movimentado` e `choque` probabilisticamente.
   - Define `estado_ativo` (`ATV`, `EMP`, `MAN`) e `observacao` de acordo com o cenário.
   - Monta um JSON com todos os campos esperados.
   - Envia um **HTTP POST** para `NODE_RED_URL`.
4. O Serial Monitor mostra:
   - O JSON enviado.
   - O código HTTP de retorno (`HTTP status: 200` em caso de sucesso).
   - A resposta do Node-RED.

Exemplo de JSON gerado:

```json
{
  "identificador_hw": "ESP32-ABC-001",
  "acc_x": -0.0220,
  "acc_y": -0.0280,
  "acc_z": 0.9970,
  "gyro_x": 0.0330,
  "gyro_y": 0.0000,
  "gyro_z": 0.0800,
  "movimentado": 1,
  "choque": 0,
  "estado_ativo": "EMP",
  "observacao": "Ativo em uso (leitura fake gerada pelo ESP32)."
}
```

---

## 7. Passo a passo para testes

### 7.1. Testar primeiro com Postman (sem ESP32)

1. Certifique-se de que o Node-RED está em execução.
2. No Postman, crie uma requisição **POST** para:

   ```text
   http://SEU_IP:1880/api/leituras-iot
   ```

3. Body → `raw` → `JSON`, com o exemplo acima.
4. Verifique:
   - A aba **Depurar** do Node-RED recebe o payload normalizado.
   - A resposta HTTP é um JSON `{ "status": "ok", ... }`.
   - No Oracle, a linha aparece em:
     ```sql
     SELECT * FROM leitura_iot ORDER BY id_leitura DESC;
     ```

### 7.2. Testar com o ESP32

1. Confirme que o **PC e o ESP32 estão na mesma rede Wi‑Fi** (mesmo range de IP).
2. Atualize `NODE_RED_URL` no `.ino` com o IP atual do PC.
3. Compile e faça o upload para o ESP32.
4. Abra o Serial Monitor.
5. Pressione o botão:
   - O Serial deve mostrar o JSON e `HTTP status: 200`.
   - A leitura deve ser gravada no Oracle, como no teste com Postman.

---

## 8. Possíveis melhorias / extensões

- Publicar as leituras também em um tópico **MQTT**.
- Criar um **dashboard** no Node-RED para visualizar as leituras em tempo real.
- Adicionar sensores físicos (acelerômetro real, giroscópio, temperatura, etc.).
- Implementar regras de negócio (alertas de choque, inatividade, manutenção).

---

## 9. Créditos

Projeto desenvolvido como parte da **Global Solution – 2º semestre (2TDS)**, integrando as disciplinas de programação, banco de dados, IoT e arquitetura de software.

