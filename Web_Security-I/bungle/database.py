import MySQLdb as mdb
from bottle import FormsDict
from hashlib import md5

# connection to database project2
def connect():
    """makes a connection to MySQL database.
    @return a mysqldb connection
    """
    
    #TODO: fill out function parameters. Use the user/password combo for the user you created in 4.1.2.1
    
    return mdb.connect(host="localhost",
                       user="cli91",
                       passwd="66fdb1cfcdad6c71191d6d03a8e128d13a6ec5bc25430d0a415748aab8527af8",
                       db="project2");

def createUser(username, password):
    """ creates a row in table named user 
    @param username: username of user
    @param password: password of user
    """

    db_rw = connect()
    cur = db_rw.cursor()

    #TODO: Implement a prepared statement using cur.execute() so that this query creates a row in table user
    create_stmt = 'INSERT INTO users VALUES (NULL, %s, %s, %s)'
    create_data = (username, password, md5(password).digest())
    cur.execute(create_stmt, create_data)

    db_rw.commit()

def validateUser(username, password):
    """ validates if username,password pair provided by user is correct or not
    @param username: username of user
    @param password: password of user
    @return True if validation was successful, False otherwise.
    """

    db_rw = connect()
    cur = db_rw.cursor()
    #TODO: Implement a prepared statement using cur.execute() so that this query selects a row from table user
    validate_stmt = 'SELECT passwordhash FROM users WHERE username = %s'
    validate_data = (username)
    cur.execute(validate_stmt, validate_data)

    if cur.rowcount < 1:
        return False

    retpwdhash = cur.fetchone()
    if md5(password).digest() != retpwdhash[0]:
        return False
    return True

def fetchUser(username):
    """ checks if there exists given username in table users or not
    if user exists return (id, username) pair
    if user does not exist return None
    @param username: the username of a user
    @return The row which has username is equal to provided input
    """

    db_rw = connect()
    cur = db_rw.cursor(mdb.cursors.DictCursor)
    print username
    #TODO: Implement a prepared statement so that this query selects a id and username of the row which has column username = username

    fetch_stmt = 'SELECT id, username FROM users WHERE username = %s'
    fetch_data = (username)
    cur.execute(fetch_stmt, fetch_data)

    if cur.rowcount < 1:
        return None
    return FormsDict(cur.fetchone())

def addHistory(user_id, query):
    """ adds a query from user with id=user_id into table named history
    @param user_id: integer id of user
    @param query: the query user has given as input
    """

    db_rw = connect()
    cur = db_rw.cursor()
    #TODO: Implement a prepared statment using cur.execute() so that this query inserts a row in table history
    addhist_stmt = 'INSERT INTO history VALUES (NULL, %s, %s)'
    addhist_data = (user_id, query)
    cur.execute(addhist_stmt, addhist_data)

    db_rw.commit()

#grabs last 15 queries made by user with id=user_id from table named history
def getHistory(user_id):
    """ grabs last 15 distinct queries made by user with id=user_id from 
    table named history
    @param user_id: integer id of user
    @return a first column of a row which MUST be query
    """

    db_rw = connect()
    cur = db_rw.cursor()
    #TODO: Implement a prepared statement using cur.execute() so that this query selects 15 distinct queries from table history
    gethist_stmt = 'SELECT DISTINCT query FROM history WHERE user_id = %s order by id desc LIMIT 15'
    gethist_data = (user_id)
    cur.execute(gethist_stmt, gethist_data)

    rows = cur.fetchall();
    return [row[0] for row in rows]
