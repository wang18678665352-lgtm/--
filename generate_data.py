#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
医院管理系统测试数据生成器 (V2 - 大规模版)
每个数据文件 100~800 条，模拟真实大型医院场景
确保所有外键引用完整性
"""
import random
import os
import hashlib
import string

DATA_DIR = r"C:\Users\王成烨\Documents\xwechat_files\wxid_mk47esqqdpw522_bfcb\msg\file\2026-04\src\data"
os.makedirs(DATA_DIR, exist_ok=True)
random.seed(20260426)

# ════════════════════════════════════════════════════════
# 基础数据池
# ════════════════════════════════════════════════════════

SURNAMES = "王李张刘陈杨赵黄周吴徐孙胡朱高林何郭马罗梁宋郑谢韩唐冯于董萧程曹袁邓许傅沈曾彭吕苏卢蒋蔡贾丁魏薛叶阎余潘杜戴夏钟汪田任姜范方石姚谭廖邹熊金陆郝孔白崔康毛邱秦江史".replace(""," ").split() + \
    "文汤尹黎易常武乔贺赖龚潘".replace(""," ").split()

GIVEN_NAMES_MALE = "伟强磊军勇杰涛斌浩鹏明辉超波刚华飞峰毅俊建国文志海斌军林平东晨宇轩子睿博一铭昊然泽哲翰诚嘉瑞安恒廷信".replace(""," ").split()
GIVEN_NAMES_FEMALE = "芳敏静丽婷娜雪琳颖慧秀娟红霞玲琴萍兰凤洁诗涵雨思怡欣悦佳琪妍萱婉若语瑶馨韵宁蓉菲".replace(""," ").split()

CITIES = [
    "北京市朝阳区","北京市海淀区","北京市西城区","北京市东城区","北京市丰台区",
    "上海市浦东新区","上海市静安区","上海市徐汇区","上海市黄浦区","上海市闵行区",
    "广州市天河区","广州市越秀区","广州市海珠区","广州市白云区","广州市番禺区",
    "深圳市南山区","深圳市福田区","深圳市罗湖区","深圳市宝安区","深圳市龙华区",
    "杭州市西湖区","杭州市拱墅区","杭州市滨江区","杭州市上城区","杭州市余杭区",
    "成都市武侯区","成都市锦江区","成都市青羊区","成都市金牛区","成都市成华区",
    "武汉市洪山区","武汉市江汉区","武汉市武昌区","武汉市江岸区","武汉市汉阳区",
    "南京市鼓楼区","南京市秦淮区","南京市玄武区","南京市建邺区","南京市江宁区",
    "重庆市渝中区","重庆市江北区","重庆市南岸区","重庆市九龙坡区","重庆市沙坪坝区",
    "西安市雁塔区","西安市碑林区","西安市未央区","西安市新城区","西安市长安区",
    "长沙市岳麓区","长沙市芙蓉区","长沙市天心区","长沙市开福区","长沙市雨花区",
    "青岛市市南区","青岛市崂山区","青岛市市北区","青岛市李沧区","青岛市黄岛区",
    "大连市中山区","大连市沙河口区","大连市西岗区","大连市甘井子区",
    "厦门市思明区","厦门市湖里区","厦门市集美区","厦门市海沧区",
    "苏州市姑苏区","苏州市吴中区","苏州市相城区","苏州市虎丘区",
    "天津市和平区","天津市南开区","天津市河西区","天津市河北区",
    "郑州市金水区","郑州市二七区","郑州市中原区","郑州市管城区",
    "济南市历下区","济南市市中区","济南市天桥区","济南市槐荫区",
    "合肥市蜀山区","合肥市庐阳区","合肥市包河区","合肥市瑶海区",
    "福州市鼓楼区","福州市台江区","福州市仓山区","福州市晋安区",
]

# 生成60个科室
DEP_NAMES = [
    "内科","外科","儿科","骨科","心内科","神经内科","呼吸内科","消化内科",
    "肾内科","内分泌科","妇产科","眼科","耳鼻喉科","口腔科","皮肤科","肿瘤科",
    "急诊科","康复科","中医科","感染科","血液科","风湿免疫科","胸外科","肝胆外科",
    "泌尿外科","神经外科","整形外科","血管外科","烧伤科","老年病科","病理科","检验科",
    "影像科","超声科","核医学科","放疗科","麻醉科","ICU","营养科","精神心理科",
    "疼痛科","介入科","透析中心","体检中心","生殖医学中心","睡眠医学中心",
    "过敏反应科","运动医学科","性病科","职业病科","预防保健科","中西医结合科",
    "全科医学科","舒缓医学科","分子医学中心","干细胞治疗中心","精准医学中心",
    "器官移植中心","微创外科中心","日间手术中心"
]
DEPARTMENTS = [(f"DEP{i+1:03d}", name) for i, name in enumerate(DEP_NAMES)]

DOCTOR_TITLES = ["主任医师","副主任医师","主治医师","住院医师","实习医生","医学博士","首席专家","返聘专家"]
PATIENT_TYPES = ["普通","医保","VIP","自费","离休干部","工伤","新农合"]
TREATMENT_STAGES = ["初诊","复诊","住院观察","术前准备","手术中","术后观察","康复观察","康复出院","转院"]
APPOINTMENT_STATUS = ["已挂号","已报到","已接诊","已完成","已取消","爽约"]
RECORD_STATUS = ["初诊记录","住院观察","治疗中","恢复中","康复观察","已出院","转科","死亡"]
ONSITE_STATUS = ["等待中","已叫号","已就诊","已完成","已取消","过号"]
CALL_TYPES = ["常规呼叫","紧急呼叫","查房确认","换药请求","检查通知","用药提醒","疼痛求助","排泄求助"]
CALL_STATUS = ["待处理","处理中","已完成","已忽略","已转交"]
REGISTRATION_QUEUE_STATUS = ["等待中","已叫号","已就诊","已完成","已取消"]

WARD_TYPES = [
    "普通病房","重症监护","VIP病房","儿科病房","骨科病房","产科病房",
    "隔离病房","日间病房","特需病房","临终关怀病房","康复病房",
    "综合病房","感染病房","神经内科病房","心血管病房"
]

DIAGNOSES = [
    ("上呼吸道感染","口服消炎药，多休息，多喝水"),
    ("急性肠胃炎","补液、止泻、注意饮食清淡"),
    ("2型糖尿病","控制饮食，规律服用降糖药，定期测血糖"),
    ("高血压","低盐饮食，规律服药，定期测量血压"),
    ("冠心病","控制血脂，抗血小板治疗，避免剧烈运动"),
    ("慢性支气管炎","戒烟，雾化吸入，抗生素治疗"),
    ("腰椎间盘突出","卧床休息，物理治疗，必要时手术"),
    ("急性阑尾炎","急诊手术切除，术后抗感染"),
    ("骨折术后康复","固定制动，逐步功能锻炼，定期复查"),
    ("小儿肺炎","抗生素治疗，雾化吸入，注意保暖"),
    ("过敏性鼻炎","抗组胺药，鼻喷激素，避免过敏原"),
    ("带状疱疹","抗病毒治疗，止痛，局部护理"),
    ("尿路感染","多饮水，抗生素治疗，注意个人卫生"),
    ("甲状腺功能亢进","抗甲状腺药物，定期复查甲功"),
    ("类风湿关节炎","抗风湿药物，物理治疗，功能锻炼"),
    ("急性心肌梗死","急诊溶栓或介入治疗，重症监护"),
    ("脑卒中后遗症","康复训练，控制危险因素，预防复发"),
    ("慢性胃炎","抑酸护胃，饮食调理，根除幽门螺杆菌"),
    ("胆囊结石","低脂饮食，必要时手术切除胆囊"),
    ("骨质疏松症","补充钙剂和维生素D，适当运动"),
    ("贫血","补充铁剂和维生素B12，加强营养，定期复查血常规"),
    ("肺结核","规范抗结核治疗，隔离防护，定期复查胸片"),
    ("肝硬化","保肝治疗，限制钠摄入，定期复查肝功能"),
    ("肾病综合征","激素治疗，低盐饮食，定期复查尿常规"),
    ("帕金森病","多巴胺替代治疗，康复训练，防跌倒"),
    ("阿尔茨海默症","认知训练，药物治疗，家属陪护"),
    ("青光眼","降眼压药物，必要时手术治疗，定期测眼压"),
    ("白内障","择期手术治疗，术后抗感染"),
    ("宫颈炎","抗感染治疗，定期妇科检查"),
    ("前列腺增生","药物治疗，必要时手术，定期复查PSA"),
    ("抑郁症","心理治疗配合药物，定期复诊评估"),
    ("焦虑症","行为治疗，必要时抗焦虑药物"),
    ("痛风","低嘌呤饮食，降尿酸药物，多饮水"),
    ("银屑病","外用激素，光疗，免疫调节"),
    ("慢性咽炎","避免刺激，雾化吸入，中药调理"),
    ("膝关节退行性变","减重，理疗，必要时关节置换"),
    ("荨麻疹","抗组胺药，避免过敏原"),
    ("胰腺炎","禁食、胃肠减压、抗感染、对症支持"),
    ("系统性红斑狼疮","激素/免疫抑制剂治疗，防晒，定期复查"),
    ("脑震荡","卧床休息，观察神志变化，对症处理"),
]

DRUG_LIST = [
    ("DR001","阿莫西林胶囊",18.50,0.70), ("DR002","布洛芬缓释片",26.00,0.60),
    ("DR003","胰岛素注射液",128.00,0.85), ("DR004","感冒灵颗粒",15.00,0.65),
    ("DR005","维生素C片",8.50,0.50), ("DR006","阿司匹林肠溶片",12.00,0.70),
    ("DR007","头孢克肟胶囊",35.00,0.75), ("DR008","蒙脱石散",22.00,0.60),
    ("DR009","奥美拉唑肠溶片",45.00,0.70), ("DR010","硝苯地平控释片",32.00,0.75),
    ("DR011","盐酸二甲双胍片",28.00,0.75), ("DR012","阿托伐他汀钙片",56.00,0.70),
    ("DR013","氯雷他定片",19.00,0.60), ("DR014","布地奈德鼻喷雾剂",68.00,0.55),
    ("DR015","葡萄糖酸钙口服液",25.00,0.50), ("DR016","复方甘草片",12.00,0.65),
    ("DR017","双氯芬酸钠缓释片",22.00,0.60), ("DR018","云南白药气雾剂",38.00,0.50),
    ("DR019","藿香正气水",9.80,0.55), ("DR020","六味地黄丸",42.00,0.50),
    ("DR021","辅酶Q10胶囊",89.00,0.50), ("DR022","葡萄糖注射液",8.00,0.80),
    ("DR023","氯化钠注射液",5.50,0.80), ("DR024","青霉素钠注射液",15.00,0.75),
    ("DR025","地塞米松注射液",12.00,0.75), ("DR026","丹参注射液",28.00,0.60),
    ("DR027","参麦注射液",45.00,0.55), ("DR028","板蓝根颗粒",16.00,0.55),
    ("DR029","连花清瘟胶囊",25.00,0.60), ("DR030","速效救心丸",35.00,0.55),
    ("DR031","硝酸甘油片",22.00,0.80), ("DR032","华法林钠片",18.00,0.75),
    ("DR033","氢氯噻嗪片",8.00,0.70), ("DR034","卡托普利片",12.00,0.75),
    ("DR035","普萘洛尔片",10.00,0.70), ("DR036","吗丁啉片",16.00,0.55),
    ("DR037","健胃消食片",11.00,0.50), ("DR038","芬必得胶囊",28.00,0.60),
    ("DR039","达克宁栓",32.00,0.50), ("DR040","红霉素软膏",8.00,0.55),
    ("DR041","左氧氟沙星片",28.00,0.70), ("DR042","氯霉素滴眼液",12.00,0.60),
    ("DR043","复方丹参滴丸",38.00,0.65), ("DR044","消渴丸",32.00,0.70),
    ("DR045","壮骨关节丸",45.00,0.50), ("DR046","三九胃泰颗粒",18.00,0.55),
    ("DR047","小儿氨酚黄那敏颗粒",12.00,0.55), ("DR048","炉甘石洗剂",8.50,0.50),
    ("DR049","藿香正气软胶囊",16.00,0.55), ("DR050","牛黄解毒片",9.50,0.50),
    ("DR051","消炎利胆片",22.00,0.55), ("DR052","银翘解毒片",11.00,0.50),
    ("DR053","珍菊降压片",24.00,0.70), ("DR054","格列齐特片",20.00,0.75),
    ("DR055","利福平胶囊",18.00,0.85), ("DR056","异烟肼片",12.00,0.85),
    ("DR057","卡马西平片",25.00,0.75), ("DR058","苯妥英钠片",15.00,0.75),
    ("DR059","地西泮片",8.00,0.70), ("DR060","多潘立酮片",16.00,0.55),
    ("DR061","马来酸氯苯那敏片",6.00,0.55), ("DR062","盐酸小檗碱片",12.00,0.60),
    ("DR063","维生素B12注射液",3.50,0.75), ("DR064","人血白蛋白",450.00,0.90),
    ("DR065","破伤风抗毒素",28.00,0.80), ("DR066","狂犬疫苗",85.00,0.70),
    ("DR067","流感疫苗",128.00,0.60), ("DR068","乙肝疫苗",78.00,0.65),
    ("DR069","葡萄糖酸锌口服液",35.00,0.50), ("DR070","碳酸钙D3片",42.00,0.55),
    ("DR071","叶酸片",8.50,0.50), ("DR072","铁剂口服液",48.00,0.55),
    ("DR073","甲硝唑片",8.00,0.55), ("DR074","替硝唑注射液",35.00,0.75),
    ("DR075","氟康唑胶囊",58.00,0.70), ("DR076","利巴韦林注射液",18.00,0.75),
    ("DR077","阿昔洛韦片",22.00,0.75), ("DR078","头孢曲松钠注射液",45.00,0.75),
    ("DR079","头孢拉定胶囊",28.00,0.70), ("DR080","罗红霉素片",22.00,0.65),
    ("DR081","克拉霉素片",35.00,0.65), ("DR082","环丙沙星片",18.00,0.70),
    ("DR083","诺氟沙星胶囊",12.00,0.60), ("DR084","甲钴胺片",38.00,0.50),
    ("DR085","胞磷胆碱钠片",56.00,0.50), ("DR086","甘露醇注射液",18.00,0.80),
    ("DR087","复方氨基酸注射液",45.00,0.75), ("DR088","脂肪乳注射液",128.00,0.75),
    ("DR089","丙泊酚注射液",89.00,0.80), ("DR090","利多卡因注射液",12.00,0.80),
    ("DR091","麻黄碱注射液",18.00,0.80), ("DR092","肾上腺素注射液",15.00,0.85),
    ("DR093","去甲肾上腺素注射液",28.00,0.85), ("DR094","多巴胺注射液",22.00,0.85),
    ("DR095","西地兰注射液",12.00,0.80), ("DR096","胺碘酮注射液",35.00,0.80),
    ("DR097","肝素钠注射液",38.00,0.80), ("DR098","尿激酶注射液",128.00,0.85),
    ("DR099","氨茶碱注射液",8.50,0.75), ("DR100","沙丁胺醇气雾剂",35.00,0.60),
    ("DR101","布地奈德混悬液",68.00,0.55), ("DR102","异丙托溴铵溶液",45.00,0.55),
    ("DR103","孟鲁司特钠片",42.00,0.60), ("DR104","氨溴索口服液",18.00,0.55),
    ("DR105","羧甲司坦片",12.00,0.55), ("DR106","缬沙坦胶囊",45.00,0.75),
    ("DR107","厄贝沙坦片",38.00,0.75), ("DR108","美托洛尔片",22.00,0.75),
    ("DR109","比索洛尔片",25.00,0.75), ("DR110","氨氯地平片",32.00,0.75),
    ("DR111","非洛地平片",28.00,0.75), ("DR112","培哚普利片",38.00,0.75),
    ("DR113","依那普利片",22.00,0.75), ("DR114","氯沙坦钾片",42.00,0.75),
    ("DR115","辛伐他汀片",35.00,0.70), ("DR116","瑞舒伐他汀钙片",58.00,0.70),
    ("DR117","非诺贝特胶囊",32.00,0.70), ("DR118","硫酸氢氯吡格雷片",68.00,0.75),
    ("DR119","阿加曲班注射液",128.00,0.80), ("DR120","艾司奥美拉唑镁肠溶片",52.00,0.70),
    ("DR121","雷贝拉唑钠肠溶片",48.00,0.70), ("DR122","铝碳酸镁咀嚼片",18.00,0.55),
    ("DR123","枸橼酸铋钾胶囊",25.00,0.60), ("DR124","胰酶肠溶胶囊",35.00,0.55),
    ("DR125","马来酸曲美布汀片",22.00,0.55), ("DR126","乳果糖口服液",38.00,0.55),
    ("DR127","聚乙二醇4000散",42.00,0.55), ("DR128","左甲状腺素钠片",35.00,0.75),
    ("DR129","甲巯咪唑片",22.00,0.75), ("DR130","丙硫氧嘧啶片",18.00,0.75),
    ("DR131","醋酸泼尼松片",12.00,0.75), ("DR132","甲泼尼龙片",38.00,0.75),
    ("DR133","硫酸羟氯喹片",68.00,0.70), ("DR134","来氟米片",58.00,0.70),
    ("DR135","甲氨蝶呤片",22.00,0.75), ("DR136","环磷酰胺注射液",45.00,0.80),
    ("DR137","卡铂注射液",128.00,0.80), ("DR138","紫杉醇注射液",860.00,0.85),
    ("DR139","吉西他滨注射液",320.00,0.85), ("DR140","顺铂注射液",45.00,0.80),
    ("DR141","氟尿嘧啶注射液",28.00,0.80), ("DR142","奥沙利铂注射液",580.00,0.85),
    ("DR143","长春新碱注射液",18.00,0.80), ("DR144","达沙替尼片",1280.00,0.85),
    ("DR145","伊马替尼胶囊",980.00,0.85), ("DR146","曲妥珠单抗注射液",3200.00,0.90),
    ("DR147","贝伐珠单抗注射液",2800.00,0.90), ("DR148","利妥昔单抗注射液",3800.00,0.90),
    ("DR149","阿达木单抗注射液",2500.00,0.85), ("DR150","英夫利西单抗注射液",2200.00,0.85),
]

# ════════════════════════════════════════════════════════
# 数量配置 - 全部 >= 100
# ════════════════════════════════════════════════════════
NUM_ADMINS   = 10
NUM_DOCTORS  = 200       # ← 40→200
NUM_PATIENTS = 500       # ← 150→500

NUM_APPOINTMENTS     = 600   # ← 200→600
NUM_RECORDS          = 500   # ← 200→500
NUM_PRESCRIPTIONS    = 600   # ← 200→600
NUM_ONSITE           = 300   # ← 150→300
NUM_WARD_CALLS       = 200   # ← 100→200
NUM_WARDS            = 100   # ← 25→100

# ════════════════════════════════════════════════════════
# Password hashing (matches C implementation: sha256(salt:password))
# ════════════════════════════════════════════════════════
def gen_salt(length=16):
    chars = string.ascii_lowercase + string.digits
    return ''.join(random.choice(chars) for _ in range(length))

def hash_password(password):
    salt = gen_salt()
    combined = f"{salt}:{password}"
    h = hashlib.sha256(combined.encode()).hexdigest()
    return f"{h}:{salt}"

# ════════════════════════════════════════════════════════
# 生成工具函数
# ════════════════════════════════════════════════════════
def random_name():
    s = random.choice(SURNAMES)
    pool = GIVEN_NAMES_MALE if random.random() < 0.48 else GIVEN_NAMES_FEMALE
    gender = "男" if pool is GIVEN_NAMES_MALE else "女"
    g = random.choice(pool)
    if random.random() < 0.5:
        g += random.choice(pool)
    return s + g, gender

def random_phone():
    pfx = random.choice(["138","139","137","136","135","158","159","188","189","150","151","152","177","186","133","155","157","181","185","130"])
    return pfx + f"{random.randint(10000000,99999999)}"

def fmt_date(ymd): return f"{ymd[0]:04d}-{ymd[1]:02d}-{ymd[2]:02d}"
def random_date(y=2026, mr=(1,5)):
    return (y, random.randint(*mr), random.randint(1,28))
def random_time():
    return f"{random.randint(7,20):02d}:{random.randint(0,59):02d}"
def random_dt(y=2026, mr=(1,5)):
    y,m,d = random_date(y, mr)
    return f"{y:04d}-{m:02d}-{d:02d} {random.randint(0,23):02d}:{random.randint(0,59):02d}:{random.randint(0,59):02d}"

# ════════════════════════════════════════════════════════
# 1. users.txt
# ════════════════════════════════════════════════════════
users = []
for i in range(1, NUM_ADMINS+1):
    pwd = f"admin{i:02d}pwd"
    users.append((f"admin{i:02d}", hash_password(pwd), "admin"))
for i in range(1, NUM_DOCTORS+1):
    pwd = f"doc{i:03d}pwd"
    users.append((f"doctor{i:03d}", hash_password(pwd), "doctor"))
for i in range(1, NUM_PATIENTS+1):
    pwd = f"pat{i:04d}pwd"
    users.append((f"patient{i:04d}", hash_password(pwd), "patient"))

with open(os.path.join(DATA_DIR, "users.txt"), "w", encoding="utf-8") as f:
    f.write("# username\tpassword\trole\n")
    for u in users:
        f.write(f"{u[0]}\t{u[1]}\t{u[2]}\n")
print(f"users.txt: {len(users)} rows")

# ════════════════════════════════════════════════════════
# 2. departments.txt (60)
# ════════════════════════════════════════════════════════
with open(os.path.join(DATA_DIR, "departments.txt"), "w", encoding="utf-8") as f:
    f.write("# department_id\tname\tleader\tphone\n")
    for dep in DEPARTMENTS:
        f.write(f"{dep[0]}\t{dep[1]}\tdoctor{random.randint(1,NUM_DOCTORS):03d}\t0755-{random.randint(1000000,9999999)}\n")
print(f"departments.txt: {len(DEPARTMENTS)} rows")

# ════════════════════════════════════════════════════════
# 3. doctors.txt (200)
# ════════════════════════════════════════════════════════
doctors = []
for i in range(1, NUM_DOCTORS+1):
    did  = f"D{i:04d}"
    uname = f"doctor{i:03d}"
    name, gender = random_name()
    dep = DEPARTMENTS[(i-1) % len(DEPARTMENTS)]
    title = random.choice(DOCTOR_TITLES)
    busy  = random.randint(0,7)
    doctors.append((did, uname, name, dep[0], title, busy))

with open(os.path.join(DATA_DIR, "doctors.txt"), "w", encoding="utf-8") as f:
    f.write("# doctor_id\tusername\tname\tdepartment_id\ttitle\tbusy_level\n")
    for d in doctors:
        f.write(f"{d[0]}\t{d[1]}\t{d[2]}\t{d[3]}\t{d[4]}\t{d[5]}\n")
print(f"doctors.txt: {len(doctors)} rows")

# ════════════════════════════════════════════════════════
# 4. patients.txt (500)
# ════════════════════════════════════════════════════════
patients = []
for i in range(1, NUM_PATIENTS+1):
    pid   = f"P{i:04d}"
    uname = f"patient{i:04d}"
    name, gender = random_name()
    age   = random.randint(1, 92)
    phone = random_phone()
    addr  = random.choice(CITIES)
    ptype = random.choice(PATIENT_TYPES)
    stage = random.choice(TREATMENT_STAGES)
    emer  = 1 if random.random() < 0.1 else 0
    patients.append((pid, uname, name, gender, age, phone, addr, ptype, stage, emer))

with open(os.path.join(DATA_DIR, "patients.txt"), "w", encoding="utf-8") as f:
    f.write("# patient_id\tusername\tname\tgender\tage\tphone\taddress\tpatient_type\ttreatment_stage\tis_emergency\n")
    for p in patients:
        f.write(f"{p[0]}\t{p[1]}\t{p[2]}\t{p[3]}\t{p[4]}\t{p[5]}\t{p[6]}\t{p[7]}\t{p[8]}\t{p[9]}\n")
print(f"patients.txt: {len(patients)} rows")

# ════════════════════════════════════════════════════════
# 5. appointments.txt (600)
# ════════════════════════════════════════════════════════
apt_ids = set()
appointments = []
for _ in range(NUM_APPOINTMENTS):
    while True:
        aid = f"APT{random.randint(10000,99999)}"
        if aid not in apt_ids: apt_ids.add(aid); break
    pt = random.choice(patients)
    dr = random.choice(doctors)
    y,m,d = random_date()
    appt_date = fmt_date((y,m,d))
    appt_time = random_time()
    st = random.choice(APPOINTMENT_STATUS)
    ct = f"{appt_date} {random.randint(7,20):02d}:{random.randint(0,59):02d}:{random.randint(0,59):02d}"
    appointments.append((aid, pt[0], dr[0], dr[3], appt_date, appt_time, st, ct))

with open(os.path.join(DATA_DIR, "appointments.txt"), "w", encoding="utf-8") as f:
    f.write("# appointment_id\tpatient_id\tdoctor_id\tdepartment_id\tappointment_date\tappointment_time\tstatus\tcreate_time\n")
    for a in appointments:
        f.write(f"{a[0]}\t{a[1]}\t{a[2]}\t{a[3]}\t{a[4]}\t{a[5]}\t{a[6]}\t{a[7]}\n")
print(f"appointments.txt: {len(appointments)} rows")

# ════════════════════════════════════════════════════════
# 6. medical_records.txt (500)
# ════════════════════════════════════════════════════════
rec_ids = set()
records = []
for _ in range(NUM_RECORDS):
    while True:
        rid = f"MR{random.randint(10000,99999)}"
        if rid not in rec_ids: rec_ids.add(rid); break
    pt = random.choice(patients)
    dr = random.choice(doctors)
    apt = random.choice(appointments)
    diag = random.choice(DIAGNOSES)
    diagnosis = f"{diag[0]}，{diag[1]}"
    y,m,d = random_date()
    dd = fmt_date((y,m,d))
    st = random.choice(RECORD_STATUS)
    records.append((rid, pt[0], dr[0], apt[0], diagnosis, dd, st))

with open(os.path.join(DATA_DIR, "medical_records.txt"), "w", encoding="utf-8") as f:
    f.write("# record_id\tpatient_id\tdoctor_id\tappointment_id\tdiagnosis\tdiagnosis_date\tstatus\n")
    for r in records:
        f.write(f"{r[0]}\t{r[1]}\t{r[2]}\t{r[3]}\t{r[4]}\t{r[5]}\t{r[6]}\n")
print(f"medical_records.txt: {len(records)} rows")

# ════════════════════════════════════════════════════════
# 7. prescriptions.txt (600)
# ════════════════════════════════════════════════════════
pr_ids = set()
prescriptions = []
for _ in range(NUM_PRESCRIPTIONS):
    while True:
        pid = f"PR{random.randint(10000,99999)}"
        if pid not in pr_ids: pr_ids.add(pid); break
    rec = random.choice(records)
    drg = random.choice(DRUG_LIST)
    qty = random.randint(1,6)
    total = round(float(drg[2]) * qty, 2)
    y,m,d = random_date()
    pd = fmt_date((y,m,d))
    prescriptions.append((pid, rec[0], rec[1], rec[2], drg[0], qty, total, pd))

with open(os.path.join(DATA_DIR, "prescriptions.txt"), "w", encoding="utf-8") as f:
    f.write("# prescription_id\trecord_id\tpatient_id\tdoctor_id\tdrug_id\tquantity\ttotal_price\tprescription_date\n")
    for p in prescriptions:
        f.write(f"{p[0]}\t{p[1]}\t{p[2]}\t{p[3]}\t{p[4]}\t{p[5]}\t{p[6]:.2f}\t{p[7]}\n")
print(f"prescriptions.txt: {len(prescriptions)} rows")

# ════════════════════════════════════════════════════════
# 8. drugs.txt (150)
# ════════════════════════════════════════════════════════
with open(os.path.join(DATA_DIR, "drugs.txt"), "w", encoding="utf-8") as f:
    f.write("# drug_id\tname\tprice\tstock_num\twarning_line\tis_special\treimbursement_ratio\n")
    for drg in DRUG_LIST:
        stock = random.randint(30, 800)
        warn  = random.randint(10, 60)
        special = 1 if float(drg[2]) > 100 or random.random() < 0.12 else 0
        f.write(f"{drg[0]}\t{drg[1]}\t{float(drg[2]):.2f}\t{stock}\t{warn}\t{special}\t{float(drg[3]):.2f}\n")
print(f"drugs.txt: {len(DRUG_LIST)} rows")

# ════════════════════════════════════════════════════════
# 9. wards.txt (100)
# ════════════════════════════════════════════════════════
wards = []
for i in range(1, NUM_WARDS+1):
    wid = f"W{i:03d}"
    wt = random.choice(WARD_TYPES)
    total = random.choice([4,6,8,10,12,15,16,20,24,30])
    remain = random.randint(0, total)
    warn = max(1, total//6)
    wards.append((wid, wt, total, remain, warn))

with open(os.path.join(DATA_DIR, "wards.txt"), "w", encoding="utf-8") as f:
    f.write("# ward_id\ttype\ttotal_beds\tremain_beds\twarning_line\n")
    for w in wards:
        f.write(f"{w[0]}\t{w[1]}\t{w[2]}\t{w[3]}\t{w[4]}\n")
print(f"wards.txt: {len(wards)} rows")

# ════════════════════════════════════════════════════════
# 10. onsite_registrations.txt (300)
# ════════════════════════════════════════════════════════
ons_ids = set()
onsites = []
for _ in range(NUM_ONSITE):
    while True:
        oid = f"ONS{random.randint(100000,999999)}"
        if oid not in ons_ids: ons_ids.add(oid); break
    pt = random.choice(patients)
    dr = random.choice(doctors)
    qn = random.randint(1, 200)
    st = random.choice(ONSITE_STATUS)
    ct = random_dt(2026, (4,5))
    onsites.append((oid, pt[0], dr[0], dr[3], qn, st, ct))

with open(os.path.join(DATA_DIR, "onsite_registrations.txt"), "w", encoding="utf-8") as f:
    f.write("# onsite_id\tpatient_id\tdoctor_id\tdepartment_id\tqueue_number\tstatus\tcreate_time\n")
    for o in onsites:
        f.write(f"{o[0]}\t{o[1]}\t{o[2]}\t{o[3]}\t{o[4]}\t{o[5]}\t{o[6]}\n")
print(f"onsite_registrations.txt: {len(onsites)} rows")

# ════════════════════════════════════════════════════════
# 11. ward_calls.txt (200)
# ════════════════════════════════════════════════════════
wc_ids = set()
ward_calls = []
for _ in range(NUM_WARD_CALLS):
    while True:
        wc = f"WC{random.randint(10000,99999)}"
        if wc not in wc_ids: wc_ids.add(wc); break
    wd = random.choice(wards)
    pt = random.choice(patients)
    dr = random.choice(doctors)
    ct = random.choice(CALL_TYPES)
    cs = random.choice(CALL_STATUS)
    notes = random.choice(["患者自述不适","护士例行查房","心率异常报警","输液完成","需要换药","疼痛加剧","呼吸困难","血压异常","体温过高","需要医生查看"])
    call_time = random_dt(2026, (4,5))
    ward_calls.append((wc, wd[0], pt[0], dr[0], ct, notes, cs, call_time))

with open(os.path.join(DATA_DIR, "ward_calls.txt"), "w", encoding="utf-8") as f:
    f.write("# call_id\tward_id\tpatient_id\tdoctor_id\tcall_type\tnotes\tstatus\tcreate_time\n")
    for w in ward_calls:
        f.write(f"{w[0]}\t{w[1]}\t{w[2]}\t{w[3]}\t{w[4]}\t{w[5]}\t{w[6]}\t{w[7]}\n")
print(f"ward_calls.txt: {len(ward_calls)} rows")

# ════════════════════════════════════════════════════════
print()
total = len(users)+len(DEPARTMENTS)+len(doctors)+len(patients)+len(appointments)+len(records)+len(prescriptions)+len(DRUG_LIST)+len(wards)+len(onsites)+len(ward_calls)
print(f"TOTAL: {total} rows across 11 files")
print("ALL DONE.")
