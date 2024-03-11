const char *selectUser = "SELECT * "
                         "FROM %s "
                         "WHERE username = '%s';";
const char *insertUser = "INSERT INTO %s (username, password) "
                         "VALUES ('%s', '%s');";
const char *selectAllArticles = "SELECT DISTINCT a.title, p.username, a.content "
                                "FROM Publishers p "
                                "JOIN Articles a ON p.ID = a.publisherID "
                                "JOIN ATPair atp ON a.ID = atp.articleID "
                                "WHERE atp.topicID IN "
                                "  (SELECT topicID "
                                "   FROM STPair "
                                "   WHERE subscriberID = %d);";
const char *selectTimestampPair = "SELECT DISTINCT lastUpdate, strftime('%%Y/%%m/%%d.%%H:%%M:%%S', 'now') "
                                  "FROM STPair "
                                  "WHERE subscriberID = %d;";
const char *selectNewArticles = "SELECT DISTINCT a.title, p.username, a.content "
                                "FROM Publishers p "
                                "JOIN Articles a ON p.ID = a.publisherID "
                                "JOIN ATPair atp ON a.ID = atp.articleID "
                                "WHERE a.datePublished > '%s' COLLATE NOCASE AND atp.topicID IN "
                                "  (SELECT topicID "
                                "   FROM STPair "
                                "   WHERE subscriberID = %d);";
const char *updateTimestamps = "UPDATE STPair "
                               "SET lastUpdate = '%s' "
                               "WHERE subscriberID = %d;";
const char *selectSTPair = "SELECT EXISTS "
                           "    (SELECT 1 FROM TOPICS t "
                           "JOIN STPair stp ON t.ID = stp.topicID "
                           "WHERE t.name = lower('%s') "
                           "AND stp.subscriberID = %d);";
const char *selectTopic = "SELECT ID "
                          "FROM Topics "
                          "WHERE name = lower('%s');";
const char *insertTopic = "INSERT INTO Topics (name) "
                          "VALUES (lower('%s'));";
const char *insertSTPair = "INSERT INTO STPair (subscriberID, topicID, lastUpdate) "
                           "VALUES (%d, %d, '0000/00/00.00:00:00');";
const char *deleteSTPair = "DELETE FROM STPair "
                           "WHERE subscriberID = %d "
                           "AND topicID = "
                           "    (SELECT ID FROM Topics "
                           "     WHERE name = lower('%s'));";
const char *selectTopics = "SELECT t.name "
                           "FROM Topics t "
                           "JOIN STPair stp ON t.ID = stp.topicID "
                           "WHERE stp.subscriberID = %d;";
const char *insertArticle = "INSERT INTO Articles (publisherID, datePublished, title, content) "
                            "VALUES (%d, strftime('%%Y/%%m/%%d.%%H:%%M:%%S', 'now'), '%s', '%s');";
const char *insertATPair = "INSERT INTO ATPair (articleID, topicID) "
                           "VALUES (%d, %d);";
const char *createSubscribers = "CREATE TABLE IF NOT EXISTS Subscribers ("
                                "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "username TEXT NOT NULL, "
                                "password TEXT NOT NULL);";
const char *createPublishers = "CREATE TABLE IF NOT EXISTS Publishers ("
                               "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "username TEXT NOT NULL, "
                               "password TEXT NOT NULL);";
const char *createArticles = "CREATE TABLE IF NOT EXISTS Articles ("
                             "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "publisherID INTEGER NOT NULL, "
                             "datePublished TEXT NOT NULL, "
                             "title TEXT NOT NULL, "
                             "content TEXT NOT NULL);";
const char *createTopics = "CREATE TABLE IF NOT EXISTS Topics ("
                           "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "name TEXT NOT NULL);";
const char *createATPair = "CREATE TABLE IF NOT EXISTS ATPair ("
                           "articleID INTEGER, "
                           "topicID INTEGER, "
                           "PRIMARY KEY (articleID, topicID));";
const char *createSTPair = "CREATE TABLE IF NOT EXISTS STPair ("
                           "subscriberID INTEGER, "
                           "topicID INTEGER, "
                           "lastUpdate TEXT NOT NULL, "
                           "PRIMARY KEY (subscriberID, topicID));";