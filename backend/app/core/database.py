import logging

from sqlalchemy import create_engine, text
from sqlalchemy.orm import declarative_base, sessionmaker

MYSQL_USER = "root"              # mysql登录用户名
MYSQL_PASSWORD = "85459939"      # mysql用户登录密码
MYSQL_HOST = "192.168.31.213"    # mysql数据库IP地址
MYSQL_PORT = 3306                # 数据库端口
MYSQL_DATABASE = "smart_home"    # 本项目需要使用的数据库名称
MYSQL_UNIX_SOCKET = "/var/run/mysqld/mysqld.sock"

LOGGER = logging.getLogger(__name__)

SQLALCHEMY_DATABASE_URL = (
    f"mysql+pymysql://{MYSQL_USER}:{MYSQL_PASSWORD}"
    f"@{MYSQL_HOST}:{MYSQL_PORT}/{MYSQL_DATABASE}"
)

SQLALCHEMY_BASE_URL = (
    f"mysql+pymysql://{MYSQL_USER}:{MYSQL_PASSWORD}"
    f"@{MYSQL_HOST}:{MYSQL_PORT}"
)

_connect_args = {}
if MYSQL_HOST in ("localhost", "127.0.0.1", "0.0.0.0"):
    _connect_args["unix_socket"] = MYSQL_UNIX_SOCKET

engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args=_connect_args,
)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

Base = declarative_base()

# 初始化mysql数据库
def init_database():
    base_engine = create_engine(
        SQLALCHEMY_BASE_URL,
        connect_args=_connect_args,
    )
    try:
        with base_engine.connect() as conn:
            conn.execute(text("COMMIT"))
            conn.execute(
                text(  # 创建数据库
                    f"CREATE DATABASE IF NOT EXISTS `{MYSQL_DATABASE}` "   # 如果没有创建就创建该数据库
                    "CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"   # 设置数据库字符集为utf8mb4，排序规则为utf8mb4_unicode_ci
                )
            )
            LOGGER.info("数据库 '%s' 已就绪", MYSQL_DATABASE)
    except Exception:
        LOGGER.exception("数据库初始化失败")
        raise
    finally:
        base_engine.dispose()


def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
