import urequests
import cellular

def get_location_radiocells():
    s = cellular.stations()
    c = dict(cellTowers=list(dict(cellId=cell_id, locationAreaCode=lac, mobileCountryCode=mcc, mobileNetworkCode=mnc) for mcc, mnc, lac, cell_id, bsic, rx_full, rx_sub, arfcn in s))
    r = urequests.post('https://backend.radiocells.org/', json=c)
    json = r.json()
    if "location" in json:
        return json["location"]["lat"], json["location"]["lng"]

def get_location_opencellid(api_key):
    s = cellular.stations()
    by_op = {}
    best = None
    best_count = 0
    for mcc, mnc, lac, cell_id, bsic, rx_full, rx_sub, arfcn in s:
        grp = mcc, mnc
        if grp not in by_op:
            by_op[grp] = []
        by_op[grp].append(dict(cid=cell_id, lac=lac))
        count = len(by_op[grp])
        if best is None or count > best_count:
            best_count = count
            best = grp

    c = dict(token=api_key, radio="gsm", mcc=best[0], mnc=best[1], cells=by_op[best])
    r = urequests.post('https://eu1.unwiredlabs.com/v2/process.php', json=c)
    json = r.json()
    if "status" in json and json["status"] == "ok":
        return json["lat"], json["lon"]

