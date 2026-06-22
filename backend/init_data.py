# from app.core.database import SessionLocal, engine
# from app.models.temperature_humidity import Base as THBase
# from app.models.electricity import Base as ElBase
# from app.models.smoke import Base as SmokeBase
# from datetime import datetime
# from random import uniform

# def init_database():
#     THBase.metadata.create_all(bind=engine)
#     ElBase.metadata.create_all(bind=engine)
#     SmokeBase.metadata.create_all(bind=engine)

#     from app.models.temperature_humidity import TemperatureHumidity
#     from app.models.electricity import ElectricityUsage

#     db = SessionLocal()

#     try:
#         if db.query(TemperatureHumidity).count() == 0:
#             print("Initializing temperature humidity data...")
#             for i in range(24):
#                 record = TemperatureHumidity(
#                     temperature=uniform(20.0, 30.0),
#                     humidity=uniform(40.0, 70.0),
#                     recorded_at=datetime.utcnow()
#                 )
#                 db.add(record)

#         if db.query(ElectricityUsage).count() == 0:
#             print("Initializing electricity usage data...")
#             for i in range(24):
#                 record = ElectricityUsage(
#                     power=uniform(300.0, 800.0),
#                     daily_usage=uniform(3.0, 8.0),
#                     recorded_at=datetime.utcnow()
#                 )
#                 db.add(record)

#         db.commit()
#         print("Database initialized successfully!")

#     except Exception as e:
#         print(f"Error initializing database: {e}")
#         db.rollback()
#     finally:
#         db.close()

# if __name__ == "__main__":
#     init_database()
