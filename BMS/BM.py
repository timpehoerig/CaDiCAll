from dataclasses import dataclass


@dataclass
class BM:
    count: int
    time: float
    real: float
    space: float
    tlim: float
    rlim: float
    slim: float
    status: float

    def __str__(self) -> str:
        return f"{self.count}\t{self.time}\t{self.real}\t{self.space}\t{self.tlim}\t{self.rlim} {self.slim}"


type BMS = dict[str, BM]
