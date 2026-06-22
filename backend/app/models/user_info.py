from sqlalchemy import Column, Integer, String
from app.core.database import Base


class UserInfo(Base):
    __tablename__ = "user_info"

    user_id = Column(Integer, primary_key=True, autoincrement=True)
    account_id = Column(String(255), nullable=False, unique=True)
    passwd = Column(String(255), nullable=False)
    authorith = Column(String(255), nullable=True)
