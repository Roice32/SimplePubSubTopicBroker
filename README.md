# SimplePubSubTopicBroker
Tehnologii utilizate: `C` (standardul `POSIX`) pentru comunicare `TCP` asincronă & concurentă, `SQLite3` pentru manipularea bazei de date locale.

### Server-ul (`Broker-ul`) acționează ca intermediar între cititori (`Subscriber-i`) și scriitori (`Publisher-i`), distribuindu-le primilor articole etichetate cu topicurile lor de interes.
### Cititorii nu sunt limitați de a fi conectați la momentul publicării unui articol, ei putând solicita primirea articolelor (toate relevante, sau doar cele necitite încă) în orice moment.

(Interfață de tip textual pentru Cititori și Scriitori pentru testare. Task-ul era realizarea serverului care realizează sarcinile menționate.)

#### Server (stânga) în timp ce Publisher-ul (dreapta) postează un articol
![Publisher Use-Case](/images/publisherUsage.png)
#### Server (stânga) în timp ce Subscriber-ul (dreapta) își actualizează topicurile preferate și cere trimiterea articolelor necitite
![Subscriber Use-Case](/images/subscriberUsage.png)
