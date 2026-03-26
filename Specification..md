# NUM8 LoRa Update Protocol Specification v1.0

## 1. Účel

Tento protokol slúži na prenos aktualizácií množiny unikátnych 8-ciferných čísel medzi:

- odosielateľom (vysielač)
- prijímačom

Aktualizácia obsahuje iba:
- zoznam čísel na odstránenie
- zoznam čísel na pridanie

Cieľ:
- čo najjednoduchší binárny protokol
- vhodný pre implementáciu v C
- vhodný pre malé dávkové update
- robustný voči duplikácii rámcov
- robustný voči poškodeniu rámcov
- kontrola verzie datasetu pred aplikáciou

---

## 2. Rozsah

Táto špecifikácia definuje:

- typy správ
- binárny frame formát
- význam polí
- poradie operácií
- validáciu
- ACK/NACK logiku
- sender state machine
- receiver state machine

Táto špecifikácia nedefinuje:

- konkrétny LoRa modul
- PHY/MAC vrstvu rádia
- šifrovanie
- LoRaWAN join
- OTA bootloader
- discovery viacerých sietí
- routovanie

---

## 3. Základné pravidlá

### 3.1 Typ dát
Každé číslo sa prenáša ako:

- `uint32_t`

Prípustné hodnoty:
- `0 .. 99999999`

### 3.2 Zakázané formy
Nepoužíva sa:
- ASCII číslo
- CSV
- JSON
- textový payload

### 3.3 Aplikačný model
Jedna aktualizácia je vždy **jeden logický batch**:

- `remove[]`
- `add[]`

Prijímač musí batch:
- aplikovať celý
- alebo ho odmietnuť celý

Nie je dovolené čiastočné potvrdenie jednej update dávky.

### 3.4 Verziovanie
Každý update musí mať:
- `dataset_version_from`
- `dataset_version_to`
- `update_id`

Prijímač smie update aplikovať len vtedy, ak:
- jeho aktuálna lokálna verzia == `dataset_version_from`

Inak musí update odmietnuť.

---

## 4. Endianness

Všetky viacbajtové číselné polia sa prenášajú ako:

- **little-endian**

---

## 5. Typy správ

Protokol v1 definuje tieto správy:

- `0x01` = BEACON
- `0x02` = UPDATE_REQUEST
- `0x03` = UPDATE_DATA
- `0x04` = ACK
- `0x05` = NACK

---

## 6. Spoločný frame header

Každý rámec začína spoločným headerom.

### 6.1 Common Header Layout

| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type         |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id      |
| 6      | 2    | uint16_t  | seq              |

### 6.2 Význam
- `protocol_version` musí byť `1`
- `msg_type` určuje typ správy
- `sender_id` je ID odosielateľa rámca
- `receiver_id` je cieľové ID, alebo `0` ak ide o broadcast
- `seq` je sekvenčné číslo rámca

### 6.3 Broadcast
Pre broadcast beacon sa používa:
- `receiver_id = 0`

---

## 7. CRC

Každý frame končí poľom:

- `uint16_t crc16`

CRC16 sa počíta cez:
- celý frame
- okrem posledných 2 bajtov CRC poľa

### 7.1 CRC algoritmus
Odporúčaný algoritmus:
- CRC-16/CCITT-FALSE

Parametre:
- poly = `0x1021`
- init = `0xFFFF`
- xorout = `0x0000`
- refin = false
- refout = false

Ak chceš iný CRC16, musí byť pevne definovaný na oboch stranách.  
Nevymýšľaj dve možnosti. Vyber jednu.

---

## 8. Typ správy: BEACON (`0x01`)

BEACON je krátka periodická broadcast správa vysielaná odosielateľom.

Účel:
- oznámiť, že existuje aktuálny dataset alebo update
- umožniť prijímaču rozhodnúť, či má vyžiadať update

### 8.1 Frame Layout

#### Common Header
| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type=0x01    |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id=0    |
| 6      | 2    | uint16_t  | seq              |

#### Payload
| Offset | Size | Type      | Name                   |
|-------:|-----:|-----------|------------------------|
| 8      | 4    | uint32_t  | current_update_id      |
| 12     | 4    | uint32_t  | dataset_version_from   |
| 16     | 4    | uint32_t  | dataset_version_to     |
| 20     | 1    | uint8_t   | remove_count           |
| 21     | 1    | uint8_t   | add_count              |
| 22     | 2    | uint16_t  | update_payload_size    |
| 24     | 2    | uint16_t  | update_payload_crc16   |

#### Footer
| Offset | Size | Type      | Name      |
|-------:|-----:|-----------|-----------|
| 26     | 2    | uint16_t  | crc16     |

### 8.2 Význam
- `current_update_id` = identifikátor aktuálneho update batchu
- `dataset_version_from` = verzia, z ktorej sa update aplikuje
- `dataset_version_to` = verzia po úspešnej aplikácii
- `remove_count` = počet čísel na vymazanie
- `add_count` = počet čísel na pridanie
- `update_payload_size` = veľkosť UPDATE_DATA payloadu bez common header a bez crc frame
- `update_payload_crc16` = CRC16 samotného update payloadu

### 8.3 Použitie
Prijímač po prijatí beaconu:
1. overí CRC frame
2. overí `protocol_version`
3. porovná `dataset_version_to` s vlastnou lokálnou verziou
4. ak update nepotrebuje, nič nerobí
5. ak update potrebuje, pošle `UPDATE_REQUEST`

---

## 9. Typ správy: UPDATE_REQUEST (`0x02`)

Prijímač touto správou žiada konkrétny update.

### 9.1 Frame Layout

#### Common Header
| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type=0x02    |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id      |
| 6      | 2    | uint16_t  | seq              |

#### Payload
| Offset | Size | Type      | Name                   |
|-------:|-----:|-----------|------------------------|
| 8      | 4    | uint32_t  | requested_update_id    |
| 12     | 4    | uint32_t  | receiver_dataset_ver   |

#### Footer
| Offset | Size | Type      | Name      |
|-------:|-----:|-----------|-----------|
| 16     | 2    | uint16_t  | crc16     |

### 9.2 Význam
- `requested_update_id` = update, ktorý chce prijímač stiahnuť
- `receiver_dataset_ver` = lokálna verzia prijímača

### 9.3 Pravidlá
Odosielateľ smie poslať `UPDATE_DATA` len ak:
- `requested_update_id` zodpovedá jeho aktuálnemu update
- `receiver_dataset_ver == dataset_version_from`

Inak musí poslať `NACK`.

---

## 10. Typ správy: UPDATE_DATA (`0x03`)

Toto je samotný update batch.

### 10.1 Základný model
UPDATE_DATA obsahuje:
- `update_id`
- `dataset_version_from`
- `dataset_version_to`
- `remove_count`
- `add_count`
- pole `remove_numbers[]`
- pole `add_numbers[]`

### 10.2 Frame Layout

#### Common Header
| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type=0x03    |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id      |
| 6      | 2    | uint16_t  | seq              |

#### Fixed Payload Header
| Offset | Size | Type      | Name                   |
|-------:|-----:|-----------|------------------------|
| 8      | 4    | uint32_t  | update_id              |
| 12     | 4    | uint32_t  | dataset_version_from   |
| 16     | 4    | uint32_t  | dataset_version_to     |
| 20     | 1    | uint8_t   | remove_count           |
| 21     | 1    | uint8_t   | add_count              |
| 22     | 2    | uint16_t  | reserved0              |

#### Variable Payload
Za týmto nasleduje:
- `uint32_t remove_numbers[remove_count]`
- `uint32_t add_numbers[add_count]`

#### Footer
Na konci celého frame:
- `uint16_t crc16`

### 10.3 reserved0
- musí byť `0`
- prijímač musí odmietnuť frame, ak nie je `0`

### 10.4 Poradie polí
Poradie je pevné:
1. fixed payload header
2. remove list
3. add list
4. crc16

### 10.5 Výpočet veľkosti payloadu
`update_payload_size` v beacon správe sa počíta ako:

- veľkosť fixed payload header = 16 bytes
- plus `remove_count * 4`
- plus `add_count * 4`

Teda:

`update_payload_size = 16 + 4*remove_count + 4*add_count`

Poznámka:
- tu sa neráta common header
- tu sa neráta footer CRC frame

### 10.6 Pravidlá validácie
Prijímač musí pred aplikáciou overiť:

1. valid frame CRC
2. `protocol_version == 1`
3. `msg_type == 0x03`
4. `receiver_id` patrí tomuto prijímaču
5. `update_id` je očakávaný
6. `dataset_version_from == local_dataset_version`
7. `dataset_version_to > dataset_version_from`
8. všetky `remove_numbers[]` sú validné `<= 99999999`
9. všetky `add_numbers[]` sú validné `<= 99999999`
10. žiadne číslo sa nesmie objaviť súčasne v `remove[]` aj v `add[]`

Ak niečo zlyhá:
- update sa nesmie aplikovať
- prijímač musí poslať `NACK`

### 10.7 Aplikačné pravidlo
Ak je frame validný, prijímač musí urobiť:

1. odstrániť všetky čísla z `remove[]`
2. pridať všetky čísla z `add[]`
3. zapísať nový `local_dataset_version = dataset_version_to`
4. zapísať `last_applied_update_id = update_id`
5. poslať `ACK`

### 10.8 Idempotencia
Ak prijímač dostane duplicitný `UPDATE_DATA` frame s:
- rovnakým `update_id`
- a už ho má aplikovaný

musí:
- znovu neposielať NACK
- ale má poslať `ACK`

To je dôležité kvôli retransmisii.

---

## 11. Typ správy: ACK (`0x04`)

ACK potvrdzuje úspešnú aplikáciu update.

### 11.1 Frame Layout

#### Common Header
| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type=0x04    |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id      |
| 6      | 2    | uint16_t  | seq              |

#### Payload
| Offset | Size | Type      | Name                 |
|-------:|-----:|-----------|----------------------|
| 8      | 4    | uint32_t  | ack_update_id        |
| 12     | 4    | uint32_t  | receiver_dataset_ver |

#### Footer
| Offset | Size | Type      | Name      |
|-------:|-----:|-----------|-----------|
| 16     | 2    | uint16_t  | crc16     |

### 11.2 Význam
- `ack_update_id` = update, ktorý bol úspešne aplikovaný
- `receiver_dataset_ver` = aktuálna verzia prijímača po aplikácii

### 11.3 Pravidlá
ACK je platný len ak:
- `ack_update_id` zodpovedá naposledy odoslanému update
- `receiver_dataset_ver == dataset_version_to`

---

## 12. Typ správy: NACK (`0x05`)

NACK informuje o chybe.

### 12.1 Frame Layout

#### Common Header
| Offset | Size | Type      | Name             |
|-------:|-----:|-----------|------------------|
| 0      | 1    | uint8_t   | protocol_version |
| 1      | 1    | uint8_t   | msg_type=0x05    |
| 2      | 2    | uint16_t  | sender_id        |
| 4      | 2    | uint16_t  | receiver_id      |
| 6      | 2    | uint16_t  | seq              |

#### Payload
| Offset | Size | Type      | Name                 |
|-------:|-----:|-----------|----------------------|
| 8      | 4    | uint32_t  | nack_update_id       |
| 12     | 2    | uint16_t  | error_code           |
| 14     | 2    | uint16_t  | detail               |

#### Footer
| Offset | Size | Type      | Name      |
|-------:|-----:|-----------|-----------|
| 16     | 2    | uint16_t  | crc16     |

### 12.2 error_code
Definované kódy:

- `1` = CRC_FAIL
- `2` = PROTOCOL_VERSION_UNSUPPORTED
- `3` = INVALID_MESSAGE_TYPE
- `4` = INVALID_RECEIVER_ID
- `5` = UPDATE_ID_UNKNOWN
- `6` = DATASET_VERSION_MISMATCH
- `7` = INVALID_NUMBER
- `8` = DUPLICATE_IN_BATCH
- `9` = NUMBER_IN_BOTH_REMOVE_AND_ADD
- `10` = APPLY_FAILED
- `11` = BUSY
- `12` = INTERNAL_ERROR

### 12.3 detail
Voliteľné pomocné pole.
Vo v1:
- ak sa nepoužíva, musí byť `0`

---

## 13. Identifikátory a verzie

### 13.1 sender_id
- unikátne ID vysielača
- `uint16_t`

### 13.2 receiver_id
- unikátne ID prijímača
- `uint16_t`

### 13.3 update_id
- identifikátor konkrétneho update batchu
- `uint32_t`
- musí byť monotónne rastúci

### 13.4 dataset_version
- verzia množiny NUM8 dát
- `uint32_t`
- musí byť monotónne rastúca

### 13.5 seq
- sekvenčné číslo rámca
- `uint16_t`
- používa sa na detekciu duplicate frame-ov v rámci linkovej relácie

`seq` nie je náhrada za `update_id`.

---

## 14. Sender state machine

### 14.1 Stav SEND_IDLE
Vysielač periodicky vysiela `BEACON`.

### 14.2 Stav SEND_WAIT_REQUEST
Po beacon vysielaní čaká na `UPDATE_REQUEST`.

### 14.3 Stav SEND_VALIDATE_REQUEST
Po prijatí requestu:
1. overí CRC
2. overí `requested_update_id`
3. overí `receiver_dataset_ver == dataset_version_from`

Ak niečo nesedí:
- pošle `NACK`
- návrat do `SEND_IDLE`

Ak sedí:
- prejde do `SEND_TRANSMIT_UPDATE`

### 14.4 Stav SEND_TRANSMIT_UPDATE
Pošle `UPDATE_DATA`.

### 14.5 Stav SEND_WAIT_ACK
Čaká na `ACK`.

Ak príde validný `ACK`:
- návrat do `SEND_IDLE`

Ak príde `NACK`:
- návrat do `SEND_IDLE`

Ak timeout:
- môže opakovať `UPDATE_DATA`
- počet retransmisií musí byť obmedzený

### 14.6 Odporúčanie
Vo v1:
- max 3 retransmisie
- potom návrat do idle

---

## 15. Receiver state machine

### 15.1 Stav RECV_SCAN
Prijímač počúva médium.

### 15.2 Stav RECV_BEACON
Po prijatí beaconu:
1. overí CRC
2. overí protocol version
3. overí sender_id
4. porovná `dataset_version_to` s lokálnou verziou

Ak update netreba:
- návrat do `RECV_SCAN`

Ak update treba:
- prejde do `RECV_REQUEST_UPDATE`

### 15.3 Stav RECV_REQUEST_UPDATE
Pošle `UPDATE_REQUEST`.

### 15.4 Stav RECV_WAIT_UPDATE
Čaká na `UPDATE_DATA`.

Ak timeout:
- môže request opakovať
- počet opakovaní musí byť obmedzený

### 15.5 Stav RECV_VALIDATE_UPDATE
Po prijatí update:
1. overí CRC
2. overí `receiver_id`
3. overí `update_id`
4. overí verziu
5. overí zoznamy čísel
6. overí konflikt remove/add

Ak niečo zlyhá:
- pošle `NACK`
- návrat do `RECV_SCAN`

Ak sedí:
- prejde do `RECV_APPLY_UPDATE`

### 15.6 Stav RECV_APPLY_UPDATE
Prijímač:
1. aplikuje remove
2. aplikuje add
3. uloží novú dataset verziu
4. uloží last_applied_update_id

Ak zlyhá:
- pošle `NACK(error=APPLY_FAILED)`
- návrat do `RECV_SCAN`

Ak uspeje:
- prejde do `RECV_SEND_ACK`

### 15.7 Stav RECV_SEND_ACK
Pošle `ACK`.

Potom:
- návrat do `RECV_SCAN`

---

## 16. Pravidlá pre zoznamy čísel

### 16.1 remove_count
- `0..255`

### 16.2 add_count
- `0..255`

### 16.3 Každé číslo
Každé číslo musí byť:
- `<= 99999999`

### 16.4 Duplicity
V rámci jedného `remove[]`:
- duplicity sú zakázané

V rámci jedného `add[]`:
- duplicity sú zakázané

Medzi `remove[]` a `add[]`:
- rovnaké číslo sa nesmie objaviť v oboch

Ak sa objaví:
- frame je neplatný
- prijímač musí poslať `NACK(NUMBER_IN_BOTH_REMOVE_AND_ADD)`

### 16.5 Zoradenie
Vo v1 nie je povinné.  
Ale odporúčané je zoradiť:
- `remove[]` vzostupne
- `add[]` vzostupne

Dôvod:
- jednoduchšia validácia
- jednoduchšie debugovanie
- jednoduchšie testy

---

## 17. Pravidlá aplikácie do NUM8

Prijímač musí update aplikovať takto:

1. validovať celý frame
2. validovať všetky čísla
3. validovať logické konflikty
4. až potom meniť NUM8 engine

Aplikácia:
1. pre každé číslo v `remove[]` zavolať remove
2. pre každé číslo v `add[]` zavolať add
3. persistnúť NUM8
4. persistnúť dataset_version
5. persistnúť last_applied_update_id

Ak persist zlyhá:
- update sa považuje za neúspešný
- pošli `NACK(APPLY_FAILED)`

Ak nevieš urobiť atómický commit (atómický zápis), musíš si to interne vyriešiť.  
To už je mimo protokol, ale nesmieš skončiť v polostave.

---

## 18. Duplicitné rámce

### 18.1 Duplicitný BEACON
Ignorovať alebo znova porovnať verziu.

### 18.2 Duplicitný UPDATE_REQUEST
Odosielateľ môže znova poslať ten istý `UPDATE_DATA`.

### 18.3 Duplicitný UPDATE_DATA
Ak už bol `update_id` úspešne aplikovaný:
- neposielať NACK
- poslať ACK

### 18.4 Duplicitný ACK
Odosielateľ môže ignorovať.

---

## 19. Timeout a retransmisia

Táto špecifikácia nedefinuje presné časy v milisekundách.  
To závisí od rádia.

Ale vo v1 musia existovať:

- timeout na čakanie `UPDATE_REQUEST`
- timeout na čakanie `UPDATE_DATA`
- timeout na čakanie `ACK`

Odporúčanie:
- max 3 opakovania
- potom zrušiť reláciu

---

## 20. Minimálne C dátové štruktúry

```c
typedef enum num8lora_msg_type_e
{
    NUM8LORA_MSG_BEACON         = 0x01,
    NUM8LORA_MSG_UPDATE_REQUEST = 0x02,
    NUM8LORA_MSG_UPDATE_DATA    = 0x03,
    NUM8LORA_MSG_ACK            = 0x04,
    NUM8LORA_MSG_NACK           = 0x05
} num8lora_msg_type_t;


typedef enum num8lora_error_e
{
    NUM8LORA_ERR_NONE                        = 0,
    NUM8LORA_ERR_CRC_FAIL                    = 1,
    NUM8LORA_ERR_PROTOCOL_VERSION_UNSUPPORTED= 2,
    NUM8LORA_ERR_INVALID_MESSAGE_TYPE        = 3,
    NUM8LORA_ERR_INVALID_RECEIVER_ID         = 4,
    NUM8LORA_ERR_UPDATE_ID_UNKNOWN           = 5,
    NUM8LORA_ERR_DATASET_VERSION_MISMATCH    = 6,
    NUM8LORA_ERR_INVALID_NUMBER              = 7,
    NUM8LORA_ERR_DUPLICATE_IN_BATCH          = 8,
    NUM8LORA_ERR_NUMBER_IN_BOTH              = 9,
    NUM8LORA_ERR_APPLY_FAILED                = 10,
    NUM8LORA_ERR_BUSY                        = 11,
    NUM8LORA_ERR_INTERNAL                    = 12
} num8lora_error_t;
typedef struct num8lora_common_header_s
{
    uint8_t  protocol_version;
    uint8_t  msg_type;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint16_t seq;
} num8lora_common_header_t;
typedef struct num8lora_beacon_payload_s
{
    uint32_t current_update_id;
    uint32_t dataset_version_from;
    uint32_t dataset_version_to;
    uint8_t  remove_count;
    uint8_t  add_count;
    uint16_t update_payload_size;
    uint16_t update_payload_crc16;
} num8lora_beacon_payload_t;
typedef struct num8lora_update_request_payload_s
{
    uint32_t requested_update_id;
    uint32_t receiver_dataset_ver;
} num8lora_update_request_payload_t;
typedef struct num8lora_update_header_s
{
    uint32_t update_id;
    uint32_t dataset_version_from;
    uint32_t dataset_version_to;
    uint8_t  remove_count;
    uint8_t  add_count;
    uint16_t reserved0;
} num8lora_update_header_t;
typedef struct num8lora_ack_payload_s
{
    uint32_t ack_update_id;
    uint32_t receiver_dataset_ver;
} num8lora_ack_payload_t;
typedef struct num8lora_nack_payload_s
{
    uint32_t nack_update_id;
    uint16_t error_code;
    uint16_t detail;
} num8lora_nack_payload_t;
21. Povinné helper funkcie pre C knižnicu

Odporúčaný minimálny kontrakt:

uint16_t num8lora_crc16_ccitt_false(const void* data, uint32_t len);

int num8lora_encode_beacon(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t seq,
    const num8lora_beacon_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_update_request(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_update_request_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_update_data(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_update_header_t* header,
    const uint32_t* remove_numbers,
    const uint32_t* add_numbers,
    uint32_t* out_len);

int num8lora_encode_ack(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_ack_payload_t* payload,
    uint32_t* out_len);

int num8lora_encode_nack(
    uint8_t* out_buf,
    uint32_t out_cap,
    uint16_t sender_id,
    uint16_t receiver_id,
    uint16_t seq,
    const num8lora_nack_payload_t* payload,
    uint32_t* out_len);

A decode/validate:

int num8lora_decode_common_header(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr);

int num8lora_validate_frame_crc(
    const uint8_t* buf,
    uint32_t len);

int num8lora_decode_beacon(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_beacon_payload_t* out_payload);

int num8lora_decode_update_request(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_update_request_payload_t* out_payload);

int num8lora_decode_update_data_header(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_update_header_t* out_upd_hdr,
    const uint8_t** out_remove_ptr,
    const uint8_t** out_add_ptr);

int num8lora_decode_ack(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_ack_payload_t* out_payload);

int num8lora_decode_nack(
    const uint8_t* buf,
    uint32_t len,
    num8lora_common_header_t* out_hdr,
    num8lora_nack_payload_t* out_payload);
22. Frame size výpočty
22.1 BEACON

common header = 8

beacon payload = 18

crc = 2

spolu = 28 bytes

22.2 UPDATE_REQUEST

common header = 8

payload = 8

crc = 2

spolu = 18 bytes

22.3 ACK

common header = 8

payload = 8

crc = 2

spolu = 18 bytes

22.4 NACK

common header = 8

payload = 8

crc = 2

spolu = 18 bytes

22.5 UPDATE_DATA

common header = 8

fixed update header = 16

remove list = 4 * remove_count

add list = 4 * add_count

crc = 2

Teda:

update_data_frame_size = 26 + 4*remove_count + 4*add_count

Príklad:

remove=20

add=20

26 + 80 + 80 = 186 bytes

23. Obmedzenia v1

Vo v1 platí:

iba jeden update batch naraz

prijímač musí mať presne očakávanú dataset_version_from

žiadne chunkovanie vnútri protokolu

žiadne šifrovanie

žiadna kompresia

žiadne partial apply

žiadne partial ACK

Ak neskôr zistíš, že potrebuješ väčšie batch-e, až potom rob v2 s chunkingom.

24. Odporúčaný behavior pri malej dávke

Pre tvoj use case:

typicky 20 remove

typicky 20 add

To znamená:

UPDATE_DATA má približne 186 bytes

protokol v1 je na to vhodný

netreba bitmap patch

netreba range patch

netreba chunking, pokiaľ ti to rádio vrstva dovolí

Ak to konkrétna LoRa vrstva nedovolí v jednom frame, potom už to nie je problém tejto logiky, ale transportu.
Vtedy spravíš v2 s chunkami. Vo v1 to nerieš.

25. Finálny záver

Tento protokol v1 je správny pre:

malé periodické update dávky

jednoduchý C kód

sender/receiver model

NUM8 engine ako cieľ aplikácie

Najdôležitejšie pravidlá:

čísla sú uint32_t

update má from_version a to_version

prijímač aplikuje batch len ak sedí from_version

každý frame má CRC16

duplicitný už aplikovaný update vracia ACK

konflikt remove/add v jednom batche je chyba

update sa aplikuje celý alebo vôbec
