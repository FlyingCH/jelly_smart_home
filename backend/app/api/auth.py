import logging

from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from sqlalchemy.orm import Session

from app.core.database import get_db
from app.models.user_info import UserInfo

router = APIRouter()
LOGGER = logging.getLogger(__name__)


class LoginRequest(BaseModel):
    account_id: str
    passwd: str


class LoginResponse(BaseModel):
    success: bool
    username: str
    message: str


@router.post("/login", response_model=LoginResponse)
async def login(data: LoginRequest, db: Session = Depends(get_db)):
    user = (
        db.query(UserInfo)
        .filter(
            UserInfo.account_id == data.account_id,
            UserInfo.passwd == data.passwd,
        )
        .first()
    )

    if not user:
        LOGGER.warning("登录失败: account_id=%s", data.account_id)
        raise HTTPException(status_code=401, detail="用户名或密码错误")

    LOGGER.info("登录成功: account_id=%s", data.account_id)
    return LoginResponse(
        success=True,
        username=user.account_id,
        message="登录成功",
    )
